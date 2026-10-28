package rendezvous

import (
	"context"
	"encoding/binary"
	"net"
	"testing"
	"time"

	"github.com/everstead/backend/internal/store"
)

type fakeLookup struct{}

func (fakeLookup) GameSessionByHostKey(ctx context.Context, hostKey string) (*store.GameSession, error) {
	if hostKey != "host-key" {
		return nil, store.ErrNotFound
	}

	return &store.GameSession{HostKey: hostKey, ExpiresAt: time.Now().Add(time.Minute)}, nil
}

func (fakeLookup) JoinTicketByText(ctx context.Context, text string) (*store.JoinTicket, error) {
	if text != "ticket" {
		return nil, store.ErrNotFound
	}

	return &store.JoinTicket{Ticket: text, SourcePid: 77, HostKey: "host-key", ExpiresAt: time.Now().Add(time.Minute)}, nil
}

func startService(t *testing.T) *net.UDPAddr {
	service, err := Listen("127.0.0.1:0", fakeLookup{})
	if err != nil {
		t.Fatalf("listen: %v", err)
	}

	t.Cleanup(func() { _ = service.Close() })
	go service.Serve()

	return service.Address().(*net.UDPAddr)
}

func dial(t *testing.T, service *net.UDPAddr) *net.UDPConn {
	connection, err := net.DialUDP("udp4", nil, service)
	if err != nil {
		t.Fatalf("dial: %v", err)
	}

	t.Cleanup(func() { _ = connection.Close() })
	return connection
}

func register(kind byte, secret string, candidates ...[6]byte) []byte {
	packet := append(header(kind), byte(len(secret)))
	packet = append(packet, secret...)
	packet = append(packet, byte(len(candidates)))
	for _, candidate := range candidates {
		packet = append(packet, candidate[:]...)
	}

	return packet
}

func receive(t *testing.T, connection *net.UDPConn, kind byte) []byte {
	buffer := make([]byte, maxPacketSize)
	_ = connection.SetReadDeadline(time.Now().Add(2 * time.Second))
	length, err := connection.Read(buffer)
	if err != nil {
		t.Fatalf("waiting for kind %d: %v", kind, err)
	}

	if length < headerSize || binary.BigEndian.Uint32(buffer) != magic || buffer[5] != kind {
		t.Fatalf("expected kind %d, got % x", kind, buffer[:length])
	}

	return buffer[headerSize:length]
}

func localPort(connection *net.UDPConn) uint16 {
	return uint16(connection.LocalAddr().(*net.UDPAddr).Port)
}

func TestIntroducesJoinerToHost(t *testing.T) {
	service := startService(t)
	hostConnection := dial(t, service)
	joinConnection := dial(t, service)

	hostCandidate := [6]byte{192, 168, 1, 10, 0x1f, 0x90}
	if _, err := hostConnection.Write(register(kindHostRegister, "host-key", hostCandidate)); err != nil {
		t.Fatalf("host register: %v", err)
	}

	registered := receive(t, hostConnection, kindHostRegistered)
	if binary.BigEndian.Uint16(registered[4:]) != localPort(hostConnection) {
		t.Fatalf("host registered with the wrong port % x", registered)
	}

	if _, err := joinConnection.Write(register(kindJoinRegister, "ticket")); err != nil {
		t.Fatalf("join register: %v", err)
	}

	introduce := receive(t, hostConnection, kindIntroduce)
	if binary.BigEndian.Uint32(introduce) != 77 {
		t.Fatalf("introduce carried pid %d", binary.BigEndian.Uint32(introduce))
	}

	token := binary.BigEndian.Uint64(introduce[4:])
	if binary.BigEndian.Uint16(introduce[16:]) != localPort(joinConnection) || introduce[18] != 0 {
		t.Fatalf("introduce carried the wrong joiner address % x", introduce)
	}

	info := receive(t, joinConnection, kindHostInfo)
	if binary.BigEndian.Uint64(info) != token {
		t.Fatal("host and joiner got different tokens")
	}

	if binary.BigEndian.Uint16(info[12:]) != localPort(hostConnection) || info[14] != 1 || [6]byte(info[15:21]) != hostCandidate {
		t.Fatalf("host info carried the wrong host address % x", info)
	}

	if _, err := joinConnection.Write(register(kindJoinRegister, "ticket")); err != nil {
		t.Fatalf("join resend: %v", err)
	}

	if binary.BigEndian.Uint64(receive(t, joinConnection, kindHostInfo)) != token {
		t.Fatal("a resent join register changed the token")
	}
}

func TestRefusesBadTicketAndOfflineHost(t *testing.T) {
	service := startService(t)
	joinConnection := dial(t, service)

	if _, err := joinConnection.Write(register(kindJoinRegister, "wrong")); err != nil {
		t.Fatalf("join register: %v", err)
	}

	if reason := receive(t, joinConnection, kindJoinRefused); reason[0] != refusedBadTicket {
		t.Fatalf("expected a bad ticket refusal, got %d", reason[0])
	}

	if _, err := joinConnection.Write(register(kindJoinRegister, "ticket")); err != nil {
		t.Fatalf("join register: %v", err)
	}

	if reason := receive(t, joinConnection, kindJoinRefused); reason[0] != refusedHostOffline {
		t.Fatalf("expected a host offline refusal, got %d", reason[0])
	}
}

func TestIgnoresUnknownHostKey(t *testing.T) {
	service := startService(t)
	hostConnection := dial(t, service)

	if _, err := hostConnection.Write(register(kindHostRegister, "guessed")); err != nil {
		t.Fatalf("host register: %v", err)
	}

	_ = hostConnection.SetReadDeadline(time.Now().Add(300 * time.Millisecond))
	if _, err := hostConnection.Read(make([]byte, maxPacketSize)); err == nil {
		t.Fatal("an unknown host key was registered")
	}
}
