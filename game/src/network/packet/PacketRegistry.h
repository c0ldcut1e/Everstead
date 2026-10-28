#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "../../io/DataInputStream.h"
#include "../../io/DataOutputStream.h"
#include "PacketIds.h"

class Packet;

class PacketRegistry
{
public:
    PacketRegistry();
    ~PacketRegistry();

    bool WritePacket(Packet *packet, std::vector<uint8_t> *outPtr);
    std::shared_ptr<Packet> ReadPacket(const std::vector<uint8_t> &bytes,
                                       bool serverPacketListener);

private:
    typedef std::function<std::shared_ptr<Packet>()> PacketCreateFunction;

    void Map(ePACKET_ID id, bool receiveOnClient, bool receiveOnServer,
             const PacketCreateFunction &createFunction);

    std::map<int, PacketCreateFunction> m_clientReceivedPackets;
    std::map<int, PacketCreateFunction> m_serverReceivedPackets;
};
