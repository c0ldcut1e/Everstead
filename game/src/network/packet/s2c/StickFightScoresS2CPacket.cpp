#include "StickFightScoresS2CPacket.h"

#include "../../listener/PacketListener.h"

StickFightScoresS2CPacket::StickFightScoresS2CPacket()
{
    m_scores.fill(-1);
}

StickFightScoresS2CPacket::StickFightScoresS2CPacket(const std::array<int, 4> &scores)
{
    m_scores = scores;
}

StickFightScoresS2CPacket::~StickFightScoresS2CPacket()
{}

ePACKET_ID StickFightScoresS2CPacket::GetId()
{
    return ePacketId_StickFightScoresS2C;
}

void StickFightScoresS2CPacket::Read(DataInputStream *inputStream)
{
    for (size_t i = 0; i < m_scores.size(); i++)
    {
        m_scores[i] = (int16_t) inputStream->ReadShort();
    }
}

void StickFightScoresS2CPacket::Write(DataOutputStream *outputStream)
{
    for (size_t i = 0; i < m_scores.size(); i++)
    {
        outputStream->WriteShort((uint16_t) m_scores[i]);
    }
}

void StickFightScoresS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleStickFightScores(this);
}

int StickFightScoresS2CPacket::GetEstimatedSize()
{
    return 8;
}

const std::array<int, 4> &StickFightScoresS2CPacket::GetScores() const
{
    return m_scores;
}
