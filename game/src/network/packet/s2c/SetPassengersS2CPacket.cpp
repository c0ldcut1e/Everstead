#include "SetPassengersS2CPacket.h"

#include "../../listener/PacketListener.h"

SetPassengersS2CPacket::SetPassengersS2CPacket()
{
    m_vehicleId    = 0;
    m_passengerIds = {};
}

SetPassengersS2CPacket::SetPassengersS2CPacket(int vehicleId, const std::vector<int> &passengerIds)
{
    m_vehicleId    = vehicleId;
    m_passengerIds = passengerIds;
}

SetPassengersS2CPacket::~SetPassengersS2CPacket()
{}

ePACKET_ID SetPassengersS2CPacket::GetId()
{
    return ePacketId_SetPassengersS2C;
}

void SetPassengersS2CPacket::Read(DataInputStream *inputStream)
{
    m_vehicleId   = (int) inputStream->ReadInt();
    uint8_t count = inputStream->ReadByte();
    m_passengerIds.resize(count);
    for (size_t i = 0; i < m_passengerIds.size(); i++)
    {
        m_passengerIds[i] = (int) inputStream->ReadInt();
    }
}

void SetPassengersS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_vehicleId);
    outputStream->WriteByte((uint8_t) m_passengerIds.size());
    for (size_t i = 0; i < m_passengerIds.size(); i++)
    {
        outputStream->WriteInt((uint32_t) m_passengerIds[i]);
    }
}

void SetPassengersS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetPassengers(this);
}

int SetPassengersS2CPacket::GetEstimatedSize()
{
    return 5 + (int) m_passengerIds.size() * 4;
}

int SetPassengersS2CPacket::GetVehicleId() const
{
    return m_vehicleId;
}

const std::vector<int> &SetPassengersS2CPacket::GetPassengerIds() const
{
    return m_passengerIds;
}
