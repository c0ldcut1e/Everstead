#pragma once

#include <mutex>
#include <string>

#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <nlohmann/json.hpp>

#include "../platform/PlatformNet.h"
#include "NetworkResult.h"

class ApiClient
{
public:
    ApiClient();
    ~ApiClient();

    ApiClient(const ApiClient &)            = delete;
    ApiClient &operator=(const ApiClient &) = delete;

    static void SetServerOverride(const std::string &server);

    void SetHost(const std::string &host, int port);
    const std::string &GetHost() const;
    int GetPort() const;

    void SetAccessToken(const std::string &accessToken);
    std::string GetAccessToken() const;
    bool HasAccessToken() const;
    const std::string &GetInstallId() const;

    NetworkResult Get(const std::string &path, nlohmann::json *outPtr);
    NetworkResult Post(const std::string &path, const nlohmann::json &body, nlohmann::json *outPtr);
    NetworkResult Put(const std::string &path, const nlohmann::json &body, nlohmann::json *outPtr);
    NetworkResult Delete(const std::string &path);

private:
    static const char *s_apiVersion;
    static const char *s_defaultHost;
    static const int s_defaultPort           = 443;
    static const int s_requestTimeoutSeconds = 10;
    static const size_t s_maxResponseBytes   = 1 << 20;

    NetworkResult Request(const char *method, const std::string &path, const std::string &body, nlohmann::json *outPtr);
    NetworkResult Connect(PlatformNet::Handle *socketOutPtr);
    bool SendAll(PlatformNet::Handle socketHandle, mbedtls_ssl_context *ssl, const std::string &data);
    bool ReceiveAll(PlatformNet::Handle socketHandle, mbedtls_ssl_context *ssl, std::string *outPtr);
    bool DecodeChunkedBody(std::string *bodyPtr);
    void GenerateInstallId();
    void ApplyServerAddress(const std::string &server);
    void LoadCaChain();

    static std::string s_serverOverride;

    mutable std::mutex m_mutex;
    std::string m_host;
    int m_port;
    bool m_secure;
    mbedtls_x509_crt m_caChain;
    std::string m_accessToken;
    std::string m_installId;
};
