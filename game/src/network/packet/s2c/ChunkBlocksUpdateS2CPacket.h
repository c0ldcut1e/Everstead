#pragma once

#include <memory>

#include "../../../world/DimensionId.h"
#include "../../../world/chunk/ChunkPos.h"
#include "../Packet.h"

class Chunk;

class ChunkBlocksUpdateS2CPacket : public Packet
{
public:
    ChunkBlocksUpdateS2CPacket();
    ChunkBlocksUpdateS2CPacket(eDIMENSION_ID dimension, const Chunk &chunk);
    ChunkBlocksUpdateS2CPacket(eDIMENSION_ID dimension, const ChunkPos &pos,
                                       const Chunk &chunk);
    ~ChunkBlocksUpdateS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    eDIMENSION_ID GetDimension() const;
    const ChunkPos &GetChunkPos() const;
    std::unique_ptr<Chunk> TakeChunk();

private:
    eDIMENSION_ID m_dimension;
    ChunkPos m_pos;
    std::unique_ptr<Chunk> m_chunk;
};
