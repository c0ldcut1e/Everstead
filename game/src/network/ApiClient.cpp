#include "ApiClient.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../platform/PlatformSockets.h"

const char *ApiClient::s_apiVersion  = "/v1";
const char *ApiClient::s_defaultHost = "5.182.204.36";
std::string ApiClient::s_serverOverride;

static const uint64_t s_fnvOffsetBasis = 0xcbf29ce484222325;
static const uint64_t s_fnvPrime       = 0x100000001b3;

ApiClient::ApiClient()
{
    m_host = s_defaultHost;
    m_port = s_defaultPort;

    const char *server = getenv("EVERSTEAD_SERVER");
    if (!s_serverOverride.empty())
    {
        ApplyServerAddress(s_serverOverride);
    }
    else if (server)
    {
        ApplyServerAddress(server);
    }

    GenerateInstallId();
}

ApiClient::~ApiClient()
{}

void ApiClient::SetServerOverride(const std::string &server)
{
    s_serverOverride = server;
}

void ApiClient::ApplyServerAddress(const std::string &server)
{
    size_t portColumn = server.rfind(':');
    if (portColumn == std::string::npos)
    {
        m_host = server;
        return;
    }

    m_host = server.substr(0, portColumn);
    m_port = atoi(server.c_str() + portColumn + 1);
}

void ApiClient::SetHost(const std::string &host, int port)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_host = host;
    m_port = port;
}

const std::string &ApiClient::GetHost() const
{
    return m_host;
}

int ApiClient::GetPort() const
{
    return m_port;
}

void ApiClient::SetAccessToken(const std::string &accessToken)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_accessToken = accessToken;
}

std::string ApiClient::GetAccessToken() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_accessToken;
}

bool ApiClient::HasAccessToken() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_accessToken.empty();
}

const std::string &ApiClient::GetInstallId() const
{
    return m_installId;
}

NetworkResult ApiClient::Get(const std::string &path, nlohmann::json *outPtr)
{
    return Request("GET", path, "", outPtr);
}

NetworkResult ApiClient::Post(const std::string &path, const nlohmann::json &body, nlohmann::json *outPtr)
{
    return Request("POST", path, body.dump(), outPtr);
}

NetworkResult ApiClient::Put(const std::string &path, const nlohmann::json &body, nlohmann::json *outPtr)
{
    return Request("PUT", path, body.dump(), outPtr);
}

NetworkResult ApiClient::Delete(const std::string &path)
{
    return Request("DELETE", path, "", nullptr);
}

NetworkResult ApiClient::Request(const char *method, const std::string &path, const std::string &body, nlohmann::json *outPtr)
{
    PlatformNet::Handle socketHandle = PlatformNet::s_invalidHandle;
    NetworkResult connectResult      = Connect(&socketHandle);
    if (connectResult.IsFailure())
    {
        return connectResult;
    }

    std::string accessToken = GetAccessToken();
    std::string request;
    request += method;
    request += ' ';
    request += s_apiVersion;
    request += path;
    request += " HTTP/1.1\r\nHost: ";
    request += m_host;
    request += ':';
    request += std::to_string(m_port);
    request += "\r\nAccept: application/json\r\nUser-Agent: Everstead\r\n";
    if (!accessToken.empty())
    {
        request += "Authorization: Bearer ";
        request += accessToken;
        request += "\r\n";
    }

    if (!body.empty())
    {
        request += "Content-Type: application/json\r\nContent-Length: ";
        request += std::to_string(body.size());
        request += "\r\n";
    }

    request += "Connection: close\r\n\r\n";
    request += body;

    if (!SendAll(socketHandle, request))
    {
        PlatformNet::Close(socketHandle);
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    std::string raw;
    bool received = ReceiveAll(socketHandle, &raw);
    PlatformNet::Close(socketHandle);
    if (!received)
    {
        return NetworkResult(eErrorGroup_Client, eClientError_Timeout);
    }

    size_t statusSpace = raw.find(' ');
    size_t headerEnd   = raw.find("\r\n\r\n");
    if (statusSpace == std::string::npos || headerEnd == std::string::npos)
    {
        return NetworkResult(eErrorGroup_Client, eClientError_BadResponse);
    }

    int statusCode      = atoi(raw.c_str() + statusSpace + 1);
    std::string headers = raw.substr(0, headerEnd);
    std::string content = raw.substr(headerEnd + 4);
    for (size_t i = 0; i < headers.size(); i++)
    {
        headers[i] = (char) tolower((unsigned char) headers[i]);
    }

    if (headers.find("transfer-encoding:") != std::string::npos && headers.find("chunked") != std::string::npos && !DecodeChunkedBody(&content))
    {
        return NetworkResult(eErrorGroup_Client, eClientError_BadResponse);
    }

    nlohmann::json document = nlohmann::json::parse(content, nullptr, false);
    if (statusCode >= 200 && statusCode < 300)
    {
        if (outPtr && statusCode != 204)
        {
            if (document.is_discarded())
            {
                return NetworkResult(eErrorGroup_Client, eClientError_BadResponse);
            }

            *outPtr = document;
        }

        return NetworkResult();
    }

    if (document.is_object() && document.contains("code") && document["code"].is_string())
    {
        return NetworkResult::FromText(document["code"].get<std::string>());
    }

    return NetworkResult(eErrorGroup_System, 0);
}

NetworkResult ApiClient::Connect(PlatformNet::Handle *socketOutPtr)
{
    if (!PlatformNet::Startup())
    {
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    std::string resolved;
    if (!PlatformNet::ResolveIpv4Address(m_host, &resolved))
    {
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    PlatformNet::Handle handle = socket(AF_INET, SOCK_STREAM, 0);
    if (!PlatformNet::IsValid(handle))
    {
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port   = htons((uint16_t) m_port);
    if (inet_pton(AF_INET, resolved.c_str(), &address.sin_addr) != 1)
    {
        PlatformNet::Close(handle);
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    PlatformNet::SetNonBlocking(handle, true);

    int connectResult = connect(handle, (sockaddr *) &address, sizeof(address));
    if (connectResult < 0 && !PlatformNet::LastErrorIsConnectPending())
    {
        PlatformNet::Close(handle);
        return NetworkResult(eErrorGroup_Client, eClientError_Connect);
    }

    if (connectResult < 0)
    {
        if (PlatformNet::WaitWritable(handle, s_requestTimeoutSeconds * 1000) <= 0)
        {
            PlatformNet::Close(handle);
            return NetworkResult(eErrorGroup_Client, eClientError_Timeout);
        }

        if (PlatformNet::GetSocketError(handle) != 0)
        {
            PlatformNet::Close(handle);
            return NetworkResult(eErrorGroup_Client, eClientError_Connect);
        }
    }

    PlatformNet::SetNonBlocking(handle, false);
    PlatformNet::SetReceiveTimeout(handle, s_requestTimeoutSeconds * 1000);
    PlatformNet::SetSendTimeout(handle, s_requestTimeoutSeconds * 1000);

    *socketOutPtr = handle;
    return NetworkResult();
}

bool ApiClient::SendAll(PlatformNet::Handle socketHandle, const std::string &data)
{
    size_t sent = 0;
    while (sent < data.size())
    {
        int64_t written = send(socketHandle, data.data() + sent, data.size() - sent, 0);
        if (written <= 0)
        {
            return false;
        }

        sent += (size_t) written;
    }

    return true;
}

bool ApiClient::ReceiveAll(PlatformNet::Handle socketHandle, std::string *outPtr)
{
    char buffer[4096];
    while (outPtr->size() < s_maxResponseBytes)
    {
        int64_t got = recv(socketHandle, buffer, sizeof(buffer), 0);
        if (got > 0)
        {
            outPtr->append(buffer, (size_t) got);
            continue;
        }

        return got == 0;
    }

    return true;
}

bool ApiClient::DecodeChunkedBody(std::string *bodyPtr)
{
    std::string decoded;
    size_t position = 0;
    while (true)
    {
        size_t lineEnd = bodyPtr->find("\r\n", position);
        if (lineEnd == std::string::npos)
        {
            return false;
        }

        std::string sizeText = bodyPtr->substr(position, lineEnd - position);
        size_t extension     = sizeText.find(';');
        if (extension != std::string::npos)
        {
            sizeText = sizeText.substr(0, extension);
        }

        while (!sizeText.empty() && isspace((unsigned char) sizeText.back()))
        {
            sizeText.pop_back();
        }

        char *parseEnd     = nullptr;
        errno              = 0;
        uint64_t chunkSize = strtoull(sizeText.c_str(), &parseEnd, 16);
        if (sizeText.empty() || parseEnd == sizeText.c_str() || *parseEnd != '\0' || errno != 0)
        {
            return false;
        }

        position = lineEnd + 2;
        if (chunkSize == 0)
        {
            break;
        }

        if (position + (size_t) chunkSize + 2 > bodyPtr->size())
        {
            return false;
        }

        decoded.append(bodyPtr->data() + position, (size_t) chunkSize);
        position += (size_t) chunkSize + 2;
    }

    *bodyPtr = decoded;
    return true;
}

void ApiClient::GenerateInstallId()
{
    std::string seed;

    const char *const environmentKeys[] = {"COMPUTERNAME", "HOSTNAME",  "HOST",     "LOGNAME", "USER",        "USERNAME",
                                           "USERDOMAIN",   "HOMEDRIVE", "HOMEPATH", "HOME",    "USERPROFILE", "PROCESSOR_IDENTIFIER"};
    for (size_t i = 0; i < sizeof(environmentKeys) / sizeof(environmentKeys[0]); i++)
    {
        const char *value = getenv(environmentKeys[i]);
        if (value)
        {
            seed += environmentKeys[i];
            seed += '=';
            seed += value;
            seed += ';';
        }
    }

    seed += "build=" __DATE__ " " __TIME__ ";";

    uint64_t high = s_fnvOffsetBasis;
    for (size_t i = 0; i < seed.size(); i++)
    {
        high ^= (uint8_t) seed[i];
        high *= s_fnvPrime;
    }

    uint64_t low = high ^ s_fnvPrime;
    for (size_t i = 0; i < seed.size(); i++)
    {
        low ^= (uint8_t) seed[i];
        low *= s_fnvPrime;
    }

    uint8_t bytes[16];
    for (int i = 0; i < 8; i++)
    {
        bytes[i]     = (uint8_t) ((high >> (56 - i * 8)) & 0xFF);
        bytes[i + 8] = (uint8_t) ((low >> (56 - i * 8)) & 0xFF);
    }

    bytes[6] = (uint8_t) ((bytes[6] & 0x0F) | 0x50);
    bytes[8] = (uint8_t) ((bytes[8] & 0x3F) | 0x80);

    char installId[37];
    snprintf(installId, sizeof(installId), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", bytes[0], bytes[1], bytes[2],
             bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    m_installId = installId;
}
