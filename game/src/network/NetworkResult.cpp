#include "NetworkResult.h"

#include <cstdio>
#include <cstdlib>

#include "../core/Localization.h"

struct KnownError
{
    uint16_t group;
    uint16_t code;
    const char *key;
};

static const KnownError s_knownErrors[] = {
        {eErrorGroup_System, 0, "network.result.generic"},
        {eErrorGroup_System, 1, "network.result.service_unavailable"},
        {eErrorGroup_System, 2, "network.result.bad_request"},
        {eErrorGroup_System, 3, "network.result.unsupported"},
        {eErrorGroup_System, 4, "network.result.not_found"},
        {eErrorGroup_System, 5, "network.result.too_large"},

        {eErrorGroup_Rate, 1, "network.result.rate_limited"},

        {eErrorGroup_Auth, 1, "network.result.sign_in_required"},
        {eErrorGroup_Auth, 2, "network.result.sign_in_again"},
        {eErrorGroup_Auth, 3, "network.result.sign_in_again"},
        {eErrorGroup_Auth, 4, "network.result.logged_in_elsewhere"},

        {eErrorGroup_Account, 1, "network.result.invalid_username"},
        {eErrorGroup_Account, 2, "network.result.password_too_short"},
        {eErrorGroup_Account, 3, "network.result.invalid_request"},
        {eErrorGroup_Account, 4, "network.result.account_exists"},
        {eErrorGroup_Account, 5, "network.result.account_not_found"},
        {eErrorGroup_Account, 6, "network.result.account_disabled"},
        {eErrorGroup_Account, 7, "network.result.invalid_credentials"},
        {eErrorGroup_Account, 8, "network.result.create_account_failed"},
        {eErrorGroup_Account, 9, "network.result.create_account_failed"},
        {eErrorGroup_Account, 10, "network.result.sign_in_failed"},
        {eErrorGroup_Account, 11, "network.result.invalid_email"},

        {eErrorGroup_Identity, 1, "network.result.account_disabled"},
        {eErrorGroup_Identity, 2, "network.result.account_disabled"},
        {eErrorGroup_Identity, 3, "network.result.update_account_failed"},
        {eErrorGroup_Identity, 4, "network.result.not_found"},
        {eErrorGroup_Identity, 5, "network.result.not_found"},
        {eErrorGroup_Identity, 6, "network.result.not_found"},

        {eErrorGroup_Social, 1, "network.result.user_not_found"},
        {eErrorGroup_Social, 2, "network.result.user_not_found"},
        {eErrorGroup_Social, 3, "network.result.add_self"},
        {eErrorGroup_Social, 4, "network.result.already_friends"},
        {eErrorGroup_Social, 5, "network.result.friend_request_not_found"},
        {eErrorGroup_Social, 6, "network.result.load_friends_failed"},
        {eErrorGroup_Social, 7, "network.result.update_friend_request_failed"},
        {eErrorGroup_Social, 8, "network.result.update_friends_failed"},
        {eErrorGroup_Social, 9, "network.result.presence_not_found"},
        {eErrorGroup_Social, 10, "network.result.update_presence_failed"},
        {eErrorGroup_Social, 11, "network.result.update_presence_failed"},

        {eErrorGroup_Session, 1, "network.result.session_not_found"},
        {eErrorGroup_Session, 2, "network.result.not_session_host"},
        {eErrorGroup_Session, 3, "network.result.session_not_joinable"},
        {eErrorGroup_Session, 4, "network.result.create_session_failed"},
        {eErrorGroup_Session, 5, "network.result.update_session_failed"},
        {eErrorGroup_Session, 6, "network.result.load_sessions_failed"},
        {eErrorGroup_Session, 7, "network.result.join_request_not_found"},
        {eErrorGroup_Session, 8, "network.result.create_join_request_failed"},
        {eErrorGroup_Session, 9, "network.result.join_request_forbidden"},
        {eErrorGroup_Session, 10, "network.result.session_player_not_found"},
        {eErrorGroup_Session, 11, "network.result.update_connection_failed"},
        {eErrorGroup_Session, 12, "network.result.load_connections_failed"},
        {eErrorGroup_Session, 13, "network.result.invalid_connection_data"},
        {eErrorGroup_Session, 14, "network.result.rendezvous_unavailable"},

        {eErrorGroup_Invite, 1, "network.result.invite_not_found"},
        {eErrorGroup_Invite, 2, "network.result.user_not_found"},
        {eErrorGroup_Invite, 3, "network.result.invite_self"},
        {eErrorGroup_Invite, 4, "network.result.update_invite_failed"},
        {eErrorGroup_Invite, 5, "network.result.load_invites_failed"},
        {eErrorGroup_Invite, 6, "network.result.invite_forbidden"},
        {eErrorGroup_Invite, 7, "network.result.invite_expired"},

        {eErrorGroup_Server, 1, "network.result.server_not_found"},
        {eErrorGroup_Server, 2, "network.result.server_name_invalid"},
        {eErrorGroup_Server, 3, "network.result.server_name_taken"},
        {eErrorGroup_Server, 4, "network.result.server_limit"},
        {eErrorGroup_Server, 5, "network.result.server_token_invalid"},
        {eErrorGroup_Server, 6, "network.result.update_server_failed"},

        {eErrorGroup_Client, eClientError_Connect, "network.result.connect_failed"},
        {eErrorGroup_Client, eClientError_Timeout, "network.result.timeout"},
        {eErrorGroup_Client, eClientError_BadResponse, "network.result.bad_response"},
        {eErrorGroup_Client, eClientError_SignedOut, "network.result.signed_out"},
        {eErrorGroup_Client, eClientError_BadArgument, "network.result.invalid_request"},
        {eErrorGroup_Client, eClientError_HostConnect, "network.result.host_connect_failed"},
        {eErrorGroup_Client, eClientError_HostTimeout, "network.result.host_timeout"},
        {eErrorGroup_Client, eClientError_ConnectionLost, "network.result.connection_lost"},
};

NetworkResult::NetworkResult()
{
    m_group = eErrorGroup_None;
    m_code  = 0;
}

NetworkResult::NetworkResult(eERROR_GROUP group, uint16_t code)
{
    m_group = group;
    m_code  = code;
}

NetworkResult NetworkResult::FromText(const std::string &text)
{
    if (text.size() != 8 || text[3] != '-')
    {
        return NetworkResult(eErrorGroup_System, 0);
    }

    return NetworkResult((eERROR_GROUP) atoi(text.substr(0, 3).c_str()), (uint16_t) atoi(text.substr(4, 4).c_str()));
}

bool NetworkResult::IsSuccess() const
{
    return m_group == eErrorGroup_None;
}

bool NetworkResult::IsFailure() const
{
    return m_group != eErrorGroup_None;
}

eERROR_GROUP NetworkResult::GetGroup() const
{
    return m_group;
}

uint16_t NetworkResult::GetCode() const
{
    return m_code;
}

std::string NetworkResult::Format() const
{
    if (IsSuccess())
    {
        return "";
    }

    char text[16];
    snprintf(text, sizeof(text), "%03u-%04u", (unsigned) m_group, (unsigned) m_code);
    return text;
}

std::wstring NetworkResult::GetMessage() const
{
    if (IsSuccess())
    {
        return L"";
    }

    for (size_t i = 0; i < sizeof(s_knownErrors) / sizeof(s_knownErrors[0]); i++)
    {
        if (s_knownErrors[i].group == m_group && s_knownErrors[i].code == m_code)
        {
            return Localization::GetInstance()->Get(s_knownErrors[i].key);
        }
    }

    return Localization::GetInstance()->Get(s_knownErrors[0].key);
}

bool NetworkResult::operator==(const NetworkResult &other) const
{
    return m_group == other.m_group && m_code == other.m_code;
}

bool NetworkResult::operator!=(const NetworkResult &other) const
{
    return !(*this == other);
}
