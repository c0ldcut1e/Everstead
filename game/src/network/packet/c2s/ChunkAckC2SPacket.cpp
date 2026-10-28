#include "ChunkAckC2SPacket.h"

#include "../../listener/PacketListener.h"

ChunkAckC2SPacket::ChunkAckC2SPacket()
{
    m_count          = 0;
    m_renderDistance = 0;
}

ChunkAckC2SPacket::ChunkAckC2SPacket(int count, int renderDistance)
{
    m_count          = count;
    m_renderDistance = renderDistance;
}

ChunkAckC2SPacket::~ChunkAckC2SPacket()
{}

ePACKET_ID ChunkAckC2SPacket::GetId()
{
    return ePacketId_ChunkAckC2S;
}

void ChunkAckC2SPacket::Read(DataInputStream *inputStream)
{
    m_count          = (int) inputStream->ReadInt();
    m_renderDistance = (int) inputStream->ReadInt();
}

void ChunkAckC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_count);
    outputStream->WriteInt((uint32_t) m_renderDistance);
}

void ChunkAckC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleChunkAck(this);
}

int ChunkAckC2SPacket::GetEstimatedSize()
{
    return 8;
}

int ChunkAckC2SPacket::GetCount() const
{
    return m_count;
}

int ChunkAckC2SPacket::GetRenderDistance() const
{
    return m_renderDistance;
}
