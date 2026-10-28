#pragma once

#include <cstdint>
#include <string>

enum eERROR_GROUP : uint16_t
{
    eErrorGroup_None     = 0,
    eErrorGroup_System   = 100,
    eErrorGroup_Rate     = 110,
    eErrorGroup_Auth     = 120,
    eErrorGroup_Account  = 200,
    eErrorGroup_Identity = 210,
    eErrorGroup_Social   = 300,
    eErrorGroup_Session  = 400,
    eErrorGroup_Invite   = 500,
    eErrorGroup_Server   = 600,
    eErrorGroup_Client   = 900,
};

enum eCLIENT_ERROR : uint16_t
{
    eClientError_Connect        = 1,
    eClientError_Timeout        = 2,
    eClientError_BadResponse    = 3,
    eClientError_SignedOut      = 4,
    eClientError_BadArgument    = 5,
    eClientError_HostConnect    = 6,
    eClientError_HostTimeout    = 7,
    eClientError_ConnectionLost = 8,
};

class NetworkResult
{
public:
    NetworkResult();
    NetworkResult(eERROR_GROUP group, uint16_t code);

    static NetworkResult FromText(const std::string &text);

    bool IsSuccess() const;
    bool IsFailure() const;

    eERROR_GROUP GetGroup() const;
    uint16_t GetCode() const;

    std::string Format() const;
    std::wstring GetMessage() const;

    bool operator==(const NetworkResult &other) const;
    bool operator!=(const NetworkResult &other) const;

private:
    eERROR_GROUP m_group;
    uint16_t m_code;
};
