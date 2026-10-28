#include "MoveVehicleC2SPacket.h"

#include "../../listener/PacketListener.h"

MoveVehicleC2SPacket::MoveVehicleC2SPacket()
{
    m_x           = 0.0;
    m_y           = 0.0;
    m_z           = 0.0;
    m_yaw         = 0.0f;
    m_leftPaddle  = false;
    m_rightPaddle = false;
}

MoveVehicleC2SPacket::MoveVehicleC2SPacket(double x, double y, double z, float yaw, bool leftPaddle, bool rightPaddle)
{
    m_x           = x;
    m_y           = y;
    m_z           = z;
    m_yaw         = yaw;
    m_leftPaddle  = leftPaddle;
    m_rightPaddle = rightPaddle;
}

MoveVehicleC2SPacket::~MoveVehicleC2SPacket()
{}

ePACKET_ID MoveVehicleC2SPacket::GetId()
{
    return ePacketId_MoveVehicleC2S;
}

void MoveVehicleC2SPacket::Read(DataInputStream *inputStream)
{
    m_x           = inputStream->ReadDouble();
    m_y           = inputStream->ReadDouble();
    m_z           = inputStream->ReadDouble();
    m_yaw         = inputStream->ReadFloat();
    m_leftPaddle  = inputStream->ReadBool();
    m_rightPaddle = inputStream->ReadBool();
}

void MoveVehicleC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteDouble(m_x);
    outputStream->WriteDouble(m_y);
    outputStream->WriteDouble(m_z);
    outputStream->WriteFloat(m_yaw);
    outputStream->WriteBool(m_leftPaddle);
    outputStream->WriteBool(m_rightPaddle);
}

void MoveVehicleC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleMoveVehicle(this);
}

int MoveVehicleC2SPacket::GetEstimatedSize()
{
    return 30;
}

double MoveVehicleC2SPacket::GetX() const
{
    return m_x;
}

double MoveVehicleC2SPacket::GetY() const
{
    return m_y;
}

double MoveVehicleC2SPacket::GetZ() const
{
    return m_z;
}

float MoveVehicleC2SPacket::GetYaw() const
{
    return m_yaw;
}

bool MoveVehicleC2SPacket::GetLeftPaddle() const
{
    return m_leftPaddle;
}

bool MoveVehicleC2SPacket::GetRightPaddle() const
{
    return m_rightPaddle;
}
