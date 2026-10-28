#include "OpenSignEditorS2CPacket.h"

#include "../../listener/PacketListener.h"

OpenSignEditorS2CPacket::OpenSignEditorS2CPacket()
{
    m_pos   = BlockPos(0, 0, 0);
    m_front = true;
}

OpenSignEditorS2CPacket::OpenSignEditorS2CPacket(const BlockPos &pos, bool front)
{
    m_pos   = pos;
    m_front = front;
}

OpenSignEditorS2CPacket::~OpenSignEditorS2CPacket()
{}

ePACKET_ID OpenSignEditorS2CPacket::GetId()
{
    return ePacketId_OpenSignEditorS2C;
}

void OpenSignEditorS2CPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);
    m_front  = inputStream->ReadByte() != 0;
}

void OpenSignEditorS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte(m_front);
}

void OpenSignEditorS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleOpenSignEditor(this);
}

int OpenSignEditorS2CPacket::GetEstimatedSize()
{
    return 13;
}

const BlockPos &OpenSignEditorS2CPacket::GetPos() const
{
    return m_pos;
}

bool OpenSignEditorS2CPacket::IsFront() const
{
    return m_front;
}
