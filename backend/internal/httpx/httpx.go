// Package httpx holds the request and response helpers every API version uses.
package httpx

import (
	"encoding/json"
	"errors"
	"io"
	"log"
	"net"
	"net/http"
	"net/netip"
	"strings"

	"github.com/everstead/backend/internal/errcode"
)

type ErrorBody struct {
	Code    string `json:"code"`
	Message string `json:"message"`
}

func WriteJSON(writer http.ResponseWriter, status int, body any) {
	writer.Header().Set("Content-Type", "application/json; charset=utf-8")
	writer.WriteHeader(status)
	if body == nil {
		return
	}

	if err := json.NewEncoder(writer).Encode(body); err != nil {
		log.Printf("write json: %v", err)
	}
}

func WriteError(writer http.ResponseWriter, apiError *errcode.Error) {
	status := apiError.Status
	if status == 0 {
		status = http.StatusInternalServerError
	}

	WriteJSON(writer, status, ErrorBody{Code: apiError.Code.String(), Message: apiError.Message})
}

func Fail(writer http.ResponseWriter, apiError *errcode.Error, cause error) {
	if cause != nil {
		log.Printf("%s: %v", apiError.Code, cause)
	}

	WriteError(writer, apiError)
}

func ReadJSON(writer http.ResponseWriter, request *http.Request, target any, maxBytes int64) bool {
	request.Body = http.MaxBytesReader(writer, request.Body, maxBytes)

	decoder := json.NewDecoder(request.Body)
	decoder.DisallowUnknownFields()
	if err := decoder.Decode(target); err != nil {
		var tooLarge *http.MaxBytesError
		if errors.As(err, &tooLarge) {
			WriteError(writer, errcode.RequestTooLarge)
			return false
		}

		WriteError(writer, errcode.BadRequestBody)
		return false
	}

	if decoder.More() {
		WriteError(writer, errcode.BadRequestBody)
		return false
	}

	return true
}

func BearerToken(request *http.Request) string {
	header := request.Header.Get("Authorization")
	if len(header) < 7 || !strings.EqualFold(header[:7], "bearer ") {
		return ""
	}

	return strings.TrimSpace(header[7:])
}

var cloudflareRanges = []netip.Prefix{
	netip.MustParsePrefix("173.245.48.0/20"),
	netip.MustParsePrefix("103.21.244.0/22"),
	netip.MustParsePrefix("103.22.200.0/22"),
	netip.MustParsePrefix("103.31.4.0/22"),
	netip.MustParsePrefix("141.101.64.0/18"),
	netip.MustParsePrefix("108.162.192.0/18"),
	netip.MustParsePrefix("190.93.240.0/20"),
	netip.MustParsePrefix("188.114.96.0/20"),
	netip.MustParsePrefix("197.234.240.0/22"),
	netip.MustParsePrefix("198.41.128.0/17"),
	netip.MustParsePrefix("162.158.0.0/15"),
	netip.MustParsePrefix("104.16.0.0/13"),
	netip.MustParsePrefix("104.24.0.0/14"),
	netip.MustParsePrefix("172.64.0.0/13"),
	netip.MustParsePrefix("131.0.72.0/22"),
	netip.MustParsePrefix("2400:cb00::/32"),
	netip.MustParsePrefix("2606:4700::/32"),
	netip.MustParsePrefix("2803:f800::/32"),
	netip.MustParsePrefix("2405:b500::/32"),
	netip.MustParsePrefix("2405:8100::/32"),
	netip.MustParsePrefix("2a06:98c0::/29"),
	netip.MustParsePrefix("2c0f:f248::/32"),
}

// Only nginx reaches us over the private docker network, and we only trust CF-Connecting-IP when
// the peer nginx saw is Cloudflare, since anyone can hit nginx on the origin IP directly
func ClientAddress(request *http.Request) string {
	host, _, err := net.SplitHostPort(request.RemoteAddr)
	if err != nil {
		return request.RemoteAddr
	}

	address, err := netip.ParseAddr(host)
	if err != nil {
		return host
	}

	if address.Unmap().IsPrivate() {
		if proxied, err := netip.ParseAddr(request.Header.Get("X-Real-IP")); err == nil {
			host = proxied.String()
			address = proxied
		}
	}

	for _, prefix := range cloudflareRanges {
		if prefix.Contains(address.Unmap()) {
			if forwarded := request.Header.Get("CF-Connecting-IP"); forwarded != "" {
				return forwarded
			}

			break
		}
	}

	return host
}

func DrainBody(request *http.Request) {
	_, _ = io.Copy(io.Discard, request.Body)
}
