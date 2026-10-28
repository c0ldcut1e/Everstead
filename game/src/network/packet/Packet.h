#pragma once

#include "../../io/DataInputStream.h"
#include "../../io/DataOutputStream.h"
#include "../../world/DynamicLights.h"
#include "../../world/item/ItemInstance.h"
#include "PacketIds.h"

class PacketListener;

class Packet
{
public:
    Packet();
    virtual ~Packet();

    virtual ePACKET_ID GetId()                         = 0;
    virtual void Read(DataInputStream *inputStream)    = 0;
    virtual void Write(DataOutputStream *outputStream) = 0;
    virtual void Handle(PacketListener *listener)      = 0;
    virtual int GetEstimatedSize();

protected:
    static const uint16_t s_emptyItemId = 0xFFFF;

    static void WriteItem(DataOutputStream *outputStream, const ItemInstance &item);
    static ItemInstance ReadItem(DataInputStream *inputStream);
    static void WriteDynamicLight(DataOutputStream *outputStream, const DynamicLights::Light &light);
    static DynamicLights::Light ReadDynamicLight(DataInputStream *inputStream);
    static void WriteFogVolume(DataOutputStream *outputStream, const DynamicLights::FogVolume &fog);
    static DynamicLights::FogVolume ReadFogVolume(DataInputStream *inputStream);
    static float ReadClampedFloat(DataInputStream *inputStream, float min, float max);
};
