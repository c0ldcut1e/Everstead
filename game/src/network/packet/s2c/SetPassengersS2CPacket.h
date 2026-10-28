#pragma once

#include <cstdint>
#include <vector>

#include "../Packet.h"

class SetPassengersS2CPacket : public Packet
{
public:
    SetPassengersS2CPacket();
    SetPassengersS2CPacket(int vehicleId, const std::vector<int> &passengerIds);
    ~SetPassengersS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetVehicleId() const;
    const std::vector<int> &GetPassengerIds() const;

private:
    int m_vehicleId;
    std::vector<int> m_passengerIds;
};
