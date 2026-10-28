package store

import (
	"crypto/rand"
	"encoding/binary"
	"encoding/hex"
)

func randomText(byteCount int) string {
	bytes := make([]byte, byteCount)
	if _, err := rand.Read(bytes); err != nil {
		panic(err)
	}

	return hex.EncodeToString(bytes)
}

func randomNumber() uint64 {
	bytes := make([]byte, 8)
	if _, err := rand.Read(bytes); err != nil {
		panic(err)
	}

	value := binary.BigEndian.Uint64(bytes) &^ (uint64(1) << 63)
	if value == 0 {
		return 1
	}

	return value
}
