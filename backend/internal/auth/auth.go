// Package auth creates and checks the credentials the API hands out.
package auth

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"net/mail"
	"regexp"

	"golang.org/x/crypto/bcrypt"
)

const MinPasswordLength = 8

var usernamePattern = regexp.MustCompile(`^[A-Za-z0-9_-]{3,16}$`)
var serverNamePattern = regexp.MustCompile(`^[A-Za-z0-9_-]{3,24}$`)

func NewToken() (token string, hash string) {
	bytes := make([]byte, 32)
	if _, err := rand.Read(bytes); err != nil {
		panic(err)
	}

	token = hex.EncodeToString(bytes)
	return token, HashToken(token)
}

func HashToken(token string) string {
	sum := sha256.Sum256([]byte(token))
	return hex.EncodeToString(sum[:])
}

func HashPassword(password string) (string, error) {
	hash, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	return string(hash), err
}

func CheckPassword(hash string, password string) bool {
	return bcrypt.CompareHashAndPassword([]byte(hash), []byte(password)) == nil
}

func ValidUsername(username string) bool {
	return usernamePattern.MatchString(username)
}

func ValidServerName(name string) bool {
	return serverNamePattern.MatchString(name)
}

func ValidEmail(email string) bool {
	address, err := mail.ParseAddress(email)
	return err == nil && address.Address == email
}

func ValidPassword(password string) bool {
	return len(password) >= MinPasswordLength
}
