// Package httpx holds the request and response helpers every API version uses.
package httpx

import (
	"encoding/json"
	"errors"
	"io"
	"log"
	"net"
	"net/http"
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

func ClientAddress(request *http.Request) string {
	if forwarded := request.Header.Get("X-Forwarded-For"); forwarded != "" {
		parts := strings.Split(forwarded, ",")
		return strings.TrimSpace(parts[0])
	}

	if real := request.Header.Get("X-Real-IP"); real != "" {
		return strings.TrimSpace(real)
	}

	host, _, err := net.SplitHostPort(request.RemoteAddr)
	if err != nil {
		return request.RemoteAddr
	}

	return host
}

func DrainBody(request *http.Request) {
	_, _ = io.Copy(io.Discard, request.Body)
}
