#pragma once

#include "../Packet.h"

class ChunkAckC2SPacket : public Packet
{
public:
    ChunkAckC2SPacket();
    ChunkAckC2SPacket(int count, int renderDistance);
    ~ChunkAckC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetCount() const;
    int GetRenderDistance() const;

private:
    int m_count;
    int m_renderDistance;
};
