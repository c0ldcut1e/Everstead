# syntax=docker/dockerfile:1
#
# Builds the Everstead account API, account website and UDP rendezvous into
# a single static binary. Build with the server directory as the context.
FROM golang:1.26-alpine AS build

WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .
RUN CGO_ENABLED=0 go build -trimpath -ldflags "-s -w" -o /out/everstead-backend ./cmd/everstead-backend

FROM alpine:3.21

RUN apk add --no-cache ca-certificates \
    && adduser --disabled-password --no-create-home --uid 10001 everstead

COPY --from=build /out/everstead-backend /usr/local/bin/everstead-backend

USER everstead
EXPOSE 8088 37373/udp

ENTRYPOINT ["/usr/local/bin/everstead-backend"]
