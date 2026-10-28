#include "DisconnectS2CPacket.h"

#include "../../../utils/StringUtils.h"
#include "../../listener/PacketListener.h"

DisconnectS2CPacket::DisconnectS2CPacket()
{
    m_reason = eDisconnectReason_None;
}

DisconnectS2CPacket::DisconnectS2CPacket(eDISCONNECT_REASON reason)
{
    m_reason = reason;
}

DisconnectS2CPacket::DisconnectS2CPacket(eDISCONNECT_REASON reason, const std::wstring &text)
{
    m_reason = reason;
    m_text   = text;
}

DisconnectS2CPacket::~DisconnectS2CPacket()
{}

ePACKET_ID DisconnectS2CPacket::GetId()
{
    return ePacketId_DisconnectS2C;
}

void DisconnectS2CPacket::Read(DataInputStream *inputStream)
{
    m_reason = (eDISCONNECT_REASON) inputStream->ReadByte();
    m_text   = StringUtils::ToUtf16(inputStream->ReadString());
}

void DisconnectS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_reason);
    outputStream->WriteString(StringUtils::ToUtf8(m_text));
}

void DisconnectS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleDisconnect(this);
}

int DisconnectS2CPacket::GetEstimatedSize()
{
    return 3 + (int) m_text.size();
}

eDISCONNECT_REASON DisconnectS2CPacket::GetReason() const
{
    return m_reason;
}

const std::wstring &DisconnectS2CPacket::GetText() const
{
    return m_text;
}

std::string DisconnectS2CPacket::GetReasonKey(eDISCONNECT_REASON reason)
{
    switch (reason)
    {
        case eDisconnectReason_Closed: {
            return "disconnect.closed";
        }

        case eDisconnectReason_LoginTooLong: {
            return "disconnect.login_too_long";
        }

        case eDisconnectReason_UnexpectedPacket: {
            return "disconnect.unexpected_packet";
        }

        case eDisconnectReason_Timeout: {
            return "disconnect.timeout";
        }

        case eDisconnectReason_MaxPlayers: {
            return "disconnect.max_players";
        }

        case eDisconnectReason_ServerStopping: {
            return "disconnect.server_stopping";
        }

        case eDisconnectReason_OlderVersion: {
            return "disconnect.older_version";
        }

        case eDisconnectReason_NewerVersion: {
            return "disconnect.newer_version";
        }

        case eDisconnectReason_SignedOut: {
            return "disconnect.signed_out";
        }

        case eDisconnectReason_SessionExpired: {
            return "disconnect.session_expired";
        }

        case eDisconnectReason_Banned: {
            return "disconnect.banned";
        }

        case eDisconnectReason_Kicked: {
            return "disconnect.kicked";
        }

        case eDisconnectReason_NotWhitelisted: {
            return "disconnect.not_whitelisted";
        }

        case eDisconnectReason_ModMismatch: {
            return "disconnect.mod_mismatch";
        }

        default: {
            return "disconnect.default";
        }
    }
}
