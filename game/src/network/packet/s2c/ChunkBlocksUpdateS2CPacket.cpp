#include "ChunkBlocksUpdateS2CPacket.h"

#include "../../../world/chunk/Chunk.h"
#include "../../../world/lighting/LightEngine.h"
#include "../../listener/PacketListener.h"

ChunkBlocksUpdateS2CPacket::ChunkBlocksUpdateS2CPacket()
{
    m_dimension = eDimensionId_Overworld;
    m_pos       = ChunkPos(0, 0);
}

ChunkBlocksUpdateS2CPacket::ChunkBlocksUpdateS2CPacket(eDIMENSION_ID dimension, const Chunk &chunk)
{
    m_dimension = dimension;
    m_pos       = chunk.GetPos();
    m_chunk     = std::make_unique<Chunk>(chunk);
}

ChunkBlocksUpdateS2CPacket::ChunkBlocksUpdateS2CPacket(eDIMENSION_ID dimension, const ChunkPos &pos, const Chunk &chunk)
{
    m_dimension = dimension;
    m_pos       = pos;
    m_chunk     = std::make_unique<Chunk>(chunk);
    m_chunk->SetPos(pos);
}

ChunkBlocksUpdateS2CPacket::~ChunkBlocksUpdateS2CPacket()
{}

ePACKET_ID ChunkBlocksUpdateS2CPacket::GetId()
{
    return ePacketId_ChunkBlocksUpdateS2C;
}

void ChunkBlocksUpdateS2CPacket::Read(DataInputStream *inputStream)
{
    m_dimension = (eDIMENSION_ID) inputStream->ReadByte();

    int posX = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    ChunkPos pos(posX, posZ);
    m_pos   = pos;
    m_chunk = std::make_unique<Chunk>(pos);

    int layerSize = Chunk::s_sizeX * Chunk::s_sizeY;
    int total     = Chunk::s_sizeZ * layerSize;

    int blockIndex = 0;
    while (blockIndex < total)
    {
        uint32_t runLength = inputStream->ReadInt();
        uint32_t blockId   = inputStream->ReadShort();
        if (runLength == 0)
        {
            m_chunk.reset();
            return;
        }

        for (uint32_t i = 0; i < runLength && blockIndex < total; i++)
        {
            int x = blockIndex % Chunk::s_sizeX;
            int y = (blockIndex / Chunk::s_sizeX) % Chunk::s_sizeY;
            int z = blockIndex / layerSize;
            m_chunk->SetBlockId(x, y, z, blockId);
            blockIndex++;
        }
    }

    int dataIndex = 0;
    while (dataIndex < total)
    {
        uint32_t runLength = inputStream->ReadInt();
        uint16_t blockData = inputStream->ReadShort();
        if (runLength == 0)
        {
            m_chunk.reset();
            return;
        }

        for (uint32_t i = 0; i < runLength && dataIndex < total; i++)
        {
            int x = dataIndex % Chunk::s_sizeX;
            int y = (dataIndex / Chunk::s_sizeX) % Chunk::s_sizeY;
            int z = dataIndex / layerSize;
            m_chunk->SetBlockData(x, y, z, blockData);
            dataIndex++;
        }
    }

    int lightIndex = 0;
    while (lightIndex < total)
    {
        uint32_t runLength = inputStream->ReadInt();
        uint8_t skyLight   = inputStream->ReadByte();
        if (runLength == 0)
        {
            m_chunk.reset();
            return;
        }

        for (uint32_t i = 0; i < runLength && lightIndex < total; i++)
        {
            int x = lightIndex % Chunk::s_sizeX;
            int y = (lightIndex / Chunk::s_sizeX) % Chunk::s_sizeY;
            int z = lightIndex / layerSize;
            m_chunk->SetSkyLight(x, y, z, skyLight);
            lightIndex++;
        }
    }

    if (!m_chunk->DecodeBiomes(inputStream->ReadBytes()))
    {
        m_chunk.reset();
        return;
    }

    LightEngine::BuildChunkBlockLight(m_chunk.get());
    m_chunk->Compact();
}

void ChunkBlocksUpdateS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_dimension);

    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.z);

    uint16_t runBlock = (uint16_t) m_chunk->GetBlockId(0, 0, 0);
    uint32_t blockRun = 0;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int y = 0; y < Chunk::s_sizeY; y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                uint16_t block = (uint16_t) m_chunk->GetBlockId(x, y, z);
                if (block != runBlock)
                {
                    outputStream->WriteInt(blockRun);
                    outputStream->WriteShort(runBlock);
                    runBlock = block;
                    blockRun = 0;
                }

                blockRun++;
            }
        }
    }

    outputStream->WriteInt(blockRun);
    outputStream->WriteShort(runBlock);

    uint16_t runData      = m_chunk->GetBlockData(0, 0, 0);
    uint32_t blockDataRun = 0;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int y = 0; y < Chunk::s_sizeY; y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                uint16_t data = m_chunk->GetBlockData(x, y, z);
                if (data != runData)
                {
                    outputStream->WriteInt(blockDataRun);
                    outputStream->WriteShort(runData);
                    runData      = data;
                    blockDataRun = 0;
                }

                blockDataRun++;
            }
        }
    }

    outputStream->WriteInt(blockDataRun);
    outputStream->WriteShort(runData);

    uint8_t runLight  = m_chunk->GetSkyLight(0, 0, 0);
    uint32_t lightRun = 0;
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int y = 0; y < Chunk::s_sizeY; y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                uint8_t skyLight = m_chunk->GetSkyLight(x, y, z);
                if (skyLight != runLight)
                {
                    outputStream->WriteInt(lightRun);
                    outputStream->WriteByte(runLight);
                    runLight = skyLight;
                    lightRun = 0;
                }

                lightRun++;
            }
        }
    }

    outputStream->WriteInt(lightRun);
    outputStream->WriteByte(runLight);

    outputStream->WriteBytes(m_chunk->EncodeBiomes());
}

void ChunkBlocksUpdateS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleChunkBlocksUpdate(this);
}

eDIMENSION_ID ChunkBlocksUpdateS2CPacket::GetDimension() const
{
    return m_dimension;
}

const ChunkPos &ChunkBlocksUpdateS2CPacket::GetChunkPos() const
{
    return m_pos;
}

std::unique_ptr<Chunk> ChunkBlocksUpdateS2CPacket::TakeChunk()
{
    return std::move(m_chunk);
}
