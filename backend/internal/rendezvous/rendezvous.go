// Package rendezvous introduces joining players to a host over UDP so both sides can punch
// through their routers and talk directly. Game traffic never passes through it.
package rendezvous

import (
	"context"
	"crypto/rand"
	"encoding/binary"
	"errors"
	"log"
	"net"
	"net/netip"
	"sync"
	"time"

	"github.com/everstead/backend/internal/store"
)

// Every packet starts with:
//
//	uint32 magic   0x45565244
//	uint8  version 1
//	uint8  kind
//
// Sent to the rendezvous from the game's own UDP socket, so the source address is the one
// other players have to aim at:
//
//	1 host register   uint8 length, host key, candidates
//	3 join register   uint8 length, ticket, candidates
//
// Sent by the rendezvous:
//
//	2 host registered uint32 ip, uint16 port the host is seen from
//	4 introduce       uint32 joiner pid, uint64 token, uint32 ip, uint16 port, candidates
//	5 host info       uint64 token, uint32 ip, uint16 port, candidates
//	6 join refused    uint8 reason, 1 = bad ticket, 2 = host not registered
//
// Candidates are uint8 count, then count times uint32 ip and uint16 port, the addresses a
// player has on its own network so players behind the same router can skip the public one.
const (
	magic           = 0x45565244
	protocolVersion = 1
	headerSize      = 6

	kindHostRegister   = 1
	kindHostRegistered = 2
	kindJoinRegister   = 3
	kindIntroduce      = 4
	kindHostInfo       = 5
	kindJoinRefused    = 6

	refusedBadTicket   = 1
	refusedHostOffline = 2

	maxCandidates   = 8
	candidateSize   = 6
	maxPacketSize   = 512
	maxHosts        = 16384
	maxJoins        = 16384
	maxLookups      = 32
	lookupTimeout   = 3 * time.Second
	hostLifetime    = 35 * time.Second
	joinLifetime    = time.Minute
	cleanupInterval = 10 * time.Second
)

// Lookup resolves the secrets players send. Both are handed out by the session API.
type Lookup interface {
	GameSessionByHostKey(ctx context.Context, hostKey string) (*store.GameSession, error)
	JoinTicketByText(ctx context.Context, text string) (*store.JoinTicket, error)
}

type Service struct {
	connection *net.UDPConn
	lookup     Lookup

	mutex     sync.Mutex
	hosts     map[string]*host
	joins     map[string]*join
	inFlight  map[string]bool
	lookups   chan struct{}
	closed    chan struct{}
	closeOnce sync.Once
}

type host struct {
	address    netip.AddrPort
	candidates []byte
	seenAt     time.Time
}

type join struct {
	hostKey   string
	pid       uint32
	token     uint64
	expiresAt time.Time
}

func Listen(address string, lookup Lookup) (*Service, error) {
	resolved, err := net.ResolveUDPAddr("udp4", address)
	if err != nil {
		return nil, err
	}

	connection, err := net.ListenUDP("udp4", resolved)
	if err != nil {
		return nil, err
	}

	return &Service{
		connection: connection,
		lookup:     lookup,
		hosts:      map[string]*host{},
		joins:      map[string]*join{},
		inFlight:   map[string]bool{},
		lookups:    make(chan struct{}, maxLookups),
		closed:     make(chan struct{}),
	}, nil
}

func (s *Service) Address() net.Addr {
	return s.connection.LocalAddr()
}

func (s *Service) Serve() error {
	go s.cleanup()

	buffer := make([]byte, maxPacketSize)
	for {
		length, source, err := s.connection.ReadFromUDPAddrPort(buffer)
		if err != nil {
			select {
			case <-s.closed:
				return nil
			default:
			}

			if errors.Is(err, net.ErrClosed) {
				return err
			}

			continue
		}

		source = netip.AddrPortFrom(source.Addr().Unmap(), source.Port())
		if !source.Addr().Is4() {
			continue
		}

		s.handle(buffer[:length], source)
	}
}

func (s *Service) Close() error {
	s.closeOnce.Do(func() { close(s.closed) })
	return s.connection.Close()
}

func (s *Service) handle(packet []byte, source netip.AddrPort) {
	if len(packet) < headerSize || binary.BigEndian.Uint32(packet) != magic || packet[4] != protocolVersion {
		return
	}

	body := packet[headerSize:]
	switch packet[5] {
	case kindHostRegister:
		key, candidates, ok := readSecretAndCandidates(body)
		if ok {
			s.registerHost(key, candidates, source)
		}
	case kindJoinRegister:
		ticket, candidates, ok := readSecretAndCandidates(body)
		if ok {
			s.registerJoin(ticket, candidates, source)
		}
	}
}

func (s *Service) registerHost(key string, candidates []byte, source netip.AddrPort) {
	s.mutex.Lock()
	known := s.hosts[key]
	if known != nil {
		known.address = source
		known.candidates = candidates
		known.seenAt = time.Now()
	}

	s.mutex.Unlock()

	if known != nil {
		s.send(source, appendAddress(header(kindHostRegistered), source))
		return
	}

	s.startLookup("host:"+key, func(ctx context.Context) {
		session, err := s.lookup.GameSessionByHostKey(ctx, key)
		if err != nil {
			logLookupError("host key", err)
			return
		}

		if !session.ExpiresAt.After(time.Now()) {
			return
		}

		s.mutex.Lock()
		if len(s.hosts) >= maxHosts {
			s.mutex.Unlock()
			return
		}

		s.hosts[key] = &host{address: source, candidates: candidates, seenAt: time.Now()}
		s.mutex.Unlock()

		s.send(source, appendAddress(header(kindHostRegistered), source))
	})
}

func (s *Service) registerJoin(ticket string, candidates []byte, source netip.AddrPort) {
	s.mutex.Lock()
	known := s.joins[ticket]
	if known != nil && known.expiresAt.After(time.Now()) {
		s.introduce(known, candidates, source)
		s.mutex.Unlock()
		return
	}

	s.mutex.Unlock()

	s.startLookup("ticket:"+ticket, func(ctx context.Context) {
		found, err := s.lookup.JoinTicketByText(ctx, ticket)
		if errors.Is(err, store.ErrNotFound) || (err == nil && !found.ExpiresAt.After(time.Now())) {
			s.send(source, append(header(kindJoinRefused), refusedBadTicket))
			return
		}

		if err != nil {
			logLookupError("ticket", err)
			return
		}

		s.mutex.Lock()
		defer s.mutex.Unlock()

		if len(s.joins) >= maxJoins {
			return
		}

		created := &join{hostKey: found.HostKey, pid: found.SourcePid, token: randomToken(), expiresAt: time.Now().Add(joinLifetime)}
		s.joins[ticket] = created
		s.introduce(created, candidates, source)
	})
}

// The caller has to hold s.mutex
func (s *Service) introduce(pending *join, candidates []byte, source netip.AddrPort) {
	target := s.hosts[pending.hostKey]
	if target == nil {
		s.send(source, append(header(kindJoinRefused), refusedHostOffline))
		return
	}

	toHost := binary.BigEndian.AppendUint32(header(kindIntroduce), pending.pid)
	toHost = binary.BigEndian.AppendUint64(toHost, pending.token)
	toHost = append(appendAddress(toHost, source), candidates...)
	s.send(target.address, toHost)

	toJoiner := binary.BigEndian.AppendUint64(header(kindHostInfo), pending.token)
	toJoiner = append(appendAddress(toJoiner, target.address), target.candidates...)
	s.send(source, toJoiner)
}

// Lookups hit the database, so they run off the read loop and the same secret is only looked
// up once at a time while its player keeps resending
func (s *Service) startLookup(name string, run func(ctx context.Context)) {
	s.mutex.Lock()
	if s.inFlight[name] {
		s.mutex.Unlock()
		return
	}

	select {
	case s.lookups <- struct{}{}:
	default:
		s.mutex.Unlock()
		return
	}

	s.inFlight[name] = true
	s.mutex.Unlock()

	go func() {
		ctx, cancel := context.WithTimeout(context.Background(), lookupTimeout)
		run(ctx)
		cancel()

		s.mutex.Lock()
		delete(s.inFlight, name)
		s.mutex.Unlock()
		<-s.lookups
	}()
}

func (s *Service) cleanup() {
	ticker := time.NewTicker(cleanupInterval)
	defer ticker.Stop()

	for {
		select {
		case <-s.closed:
			return
		case now := <-ticker.C:
			s.mutex.Lock()
			for key, registered := range s.hosts {
				if now.Sub(registered.seenAt) > hostLifetime {
					delete(s.hosts, key)
				}
			}

			for ticket, pending := range s.joins {
				if now.After(pending.expiresAt) {
					delete(s.joins, ticket)
				}
			}

			s.mutex.Unlock()
		}
	}
}

func (s *Service) send(target netip.AddrPort, packet []byte) {
	_, _ = s.connection.WriteToUDPAddrPort(packet, target)
}

func header(kind byte) []byte {
	packet := make([]byte, 0, 96)
	packet = binary.BigEndian.AppendUint32(packet, magic)
	return append(packet, protocolVersion, kind)
}

func appendAddress(packet []byte, address netip.AddrPort) []byte {
	ip := address.Addr().As4()
	packet = append(packet, ip[:]...)
	return binary.BigEndian.AppendUint16(packet, address.Port())
}

// Candidates are returned still encoded since they are only ever forwarded
func readSecretAndCandidates(body []byte) (string, []byte, bool) {
	if len(body) < 1 {
		return "", nil, false
	}

	length := int(body[0])
	if length == 0 || len(body) < 1+length+1 {
		return "", nil, false
	}

	secret := string(body[1 : 1+length])
	candidates := body[1+length:]
	count := int(candidates[0])
	if count > maxCandidates || len(candidates) != 1+count*candidateSize {
		return "", nil, false
	}

	return secret, append([]byte(nil), candidates...), true
}

func randomToken() uint64 {
	bytes := make([]byte, 8)
	if _, err := rand.Read(bytes); err != nil {
		panic(err)
	}

	return binary.BigEndian.Uint64(bytes)
}

func logLookupError(what string, err error) {
	if !errors.Is(err, store.ErrNotFound) {
		log.Printf("rendezvous %s lookup failed: %v", what, err)
	}
}
