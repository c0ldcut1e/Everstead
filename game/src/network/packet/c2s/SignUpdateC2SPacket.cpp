#include "SignUpdateC2SPacket.h"

#include "../../listener/PacketListener.h"

SignUpdateC2SPacket::SignUpdateC2SPacket()
{
    m_pos   = BlockPos(0, 0, 0);
    m_front = true;
}

SignUpdateC2SPacket::SignUpdateC2SPacket(const BlockPos &pos, bool front, const std::string *lines)
{
    m_pos   = pos;
    m_front = front;
    for (int i = 0; i < 4; i++)
    {
        m_lines[i] = lines[i];
    }
}

SignUpdateC2SPacket::~SignUpdateC2SPacket()
{}

ePACKET_ID SignUpdateC2SPacket::GetId()
{
    return ePacketId_SignUpdateC2S;
}

void SignUpdateC2SPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);
    m_front  = inputStream->ReadByte() != 0;
    for (int i = 0; i < 4; i++)
    {
        m_lines[i] = inputStream->ReadString();
    }
}

void SignUpdateC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte(m_front);
    for (int i = 0; i < 4; i++)
    {
        outputStream->WriteString(m_lines[i]);
    }
}

void SignUpdateC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleSignUpdate(this);
}

int SignUpdateC2SPacket::GetEstimatedSize()
{
    int size = 13;
    for (int i = 0; i < 4; i++)
    {
        size += 2 + (int) m_lines[i].size();
    }

    return size;
}

const BlockPos &SignUpdateC2SPacket::GetPos() const
{
    return m_pos;
}

bool SignUpdateC2SPacket::IsFront() const
{
    return m_front;
}

const std::string &SignUpdateC2SPacket::GetLine(int line) const
{
    return m_lines[line];
}
