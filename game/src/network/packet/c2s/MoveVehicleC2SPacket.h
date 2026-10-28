#pragma once

#include <cstdint>

#include "../Packet.h"

class MoveVehicleC2SPacket : public Packet
{
public:
    MoveVehicleC2SPacket();
    MoveVehicleC2SPacket(double x, double y, double z, float yaw, bool leftPaddle, bool rightPaddle);
    ~MoveVehicleC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    double GetX() const;
    double GetY() const;
    double GetZ() const;
    float GetYaw() const;
    bool GetLeftPaddle() const;
    bool GetRightPaddle() const;

private:
    double m_x;
    double m_y;
    double m_z;
    float m_yaw;
    bool m_leftPaddle;
    bool m_rightPaddle;
};
