#include "Packet.h"

#include <algorithm>
#include <cmath>

#include "../../tag/NbtIo.h"
#include "../../world/item/Item.h"

#include <glm/geometric.hpp>

Packet::Packet()
{}

Packet::~Packet()
{}

int Packet::GetEstimatedSize()
{
    return 0;
}

void Packet::WriteItem(DataOutputStream *outputStream, const ItemInstance &item)
{
    if (item.IsEmpty() || !item.GetItem())
    {
        outputStream->WriteShort(s_emptyItemId);
        return;
    }

    outputStream->WriteShort((uint16_t) item.GetItem()->GetId());
    outputStream->WriteByte((uint8_t) item.GetCount());
    outputStream->WriteShort((uint16_t) item.GetAuxValue());
    outputStream->WriteBytes(item.GetTag() ? NbtIo::WriteBytes(item.GetTag()) : std::vector<uint8_t>());
}

ItemInstance Packet::ReadItem(DataInputStream *inputStream)
{
    uint16_t itemId = inputStream->ReadShort();
    if (itemId == s_emptyItemId)
    {
        return ItemInstance();
    }

    int count                = (int) inputStream->ReadByte();
    int aux                  = (int) inputStream->ReadShort();
    std::vector<uint8_t> tag = inputStream->ReadBytes();

    Item *item = Item::ById(itemId);
    if (!item || count <= 0)
    {
        return ItemInstance();
    }

    if (count > item->GetMaxStackSize())
    {
        count = item->GetMaxStackSize();
    }

    ItemInstance instance(item, count, aux);
    if (!tag.empty())
    {
        instance.SetTag(NbtIo::ReadBytes(tag).release());
    }

    return instance;
}

void Packet::WriteDynamicLight(DataOutputStream *outputStream, const DynamicLights::Light &light)
{
    outputStream->WriteInt(light.id);
    outputStream->WriteByte((uint8_t) light.type);
    for (int i = 0; i < 3; i++)
    {
        outputStream->WriteDouble(light.position[i]);
        outputStream->WriteFloat(light.direction[i]);
        outputStream->WriteFloat(light.color[i]);
    }

    outputStream->WriteFloat(light.intensity);
    outputStream->WriteFloat(light.radius);
    outputStream->WriteFloat(light.coneAngle);
    outputStream->WriteFloat(light.size);
    outputStream->WriteBool(light.volumetric);
    outputStream->WriteBool(light.occlusion);
}

DynamicLights::Light Packet::ReadDynamicLight(DataInputStream *inputStream)
{
    const double maxCoordinate = 30000000.0;

    DynamicLights::Light light;
    light.id   = inputStream->ReadInt();
    light.type = (DynamicLights::eLIGHT_TYPE) std::min((int) inputStream->ReadByte(), (int) DynamicLights::eLightType_Directional);
    for (int i = 0; i < 3; i++)
    {
        double position    = inputStream->ReadDouble();
        light.position[i]  = std::isfinite(position) ? std::clamp(position, -maxCoordinate, maxCoordinate) : 0.0;
        light.direction[i] = ReadClampedFloat(inputStream, -1.0f, 1.0f);
        light.color[i]     = ReadClampedFloat(inputStream, 0.0f, 1.0f);
    }

    light.intensity  = ReadClampedFloat(inputStream, 0.0f, 8.0f);
    light.radius     = ReadClampedFloat(inputStream, 1.0f, 64.0f);
    light.coneAngle  = ReadClampedFloat(inputStream, 1.0f, 179.0f);
    light.size       = ReadClampedFloat(inputStream, 0.0f, 4.0f);
    light.volumetric = inputStream->ReadBool();
    light.occlusion  = inputStream->ReadBool();
    if (glm::dot(light.direction, light.direction) < 0.0001f)
    {
        light.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    }

    return light;
}

void Packet::WriteFogVolume(DataOutputStream *outputStream, const DynamicLights::FogVolume &fog)
{
    outputStream->WriteInt(fog.id);
    outputStream->WriteByte((uint8_t) fog.shape);
    for (int i = 0; i < 3; i++)
    {
        outputStream->WriteDouble(fog.position[i]);
        outputStream->WriteFloat(fog.extent[i]);
        outputStream->WriteFloat(fog.color[i]);
    }

    outputStream->WriteFloat(fog.density);
    outputStream->WriteFloat(fog.edge);
    outputStream->WriteFloat(fog.noise);
    outputStream->WriteFloat(fog.drift);
    outputStream->WriteFloat(fog.ambient);
}

DynamicLights::FogVolume Packet::ReadFogVolume(DataInputStream *inputStream)
{
    const double maxCoordinate = 30000000.0;

    DynamicLights::FogVolume fog;
    fog.id    = inputStream->ReadInt();
    fog.shape = (DynamicLights::eFOG_SHAPE) std::min((int) inputStream->ReadByte(), (int) DynamicLights::eFogShape_Sphere);
    for (int i = 0; i < 3; i++)
    {
        double position = inputStream->ReadDouble();
        fog.position[i] = std::isfinite(position) ? std::clamp(position, -maxCoordinate, maxCoordinate) : 0.0;
        fog.extent[i]   = ReadClampedFloat(inputStream, 0.5f, 64.0f);
        fog.color[i]    = ReadClampedFloat(inputStream, 0.0f, 1.0f);
    }

    fog.density = ReadClampedFloat(inputStream, 0.0f, 1.0f);
    fog.edge    = ReadClampedFloat(inputStream, 0.0f, 1.0f);
    fog.noise   = ReadClampedFloat(inputStream, 0.0f, 1.0f);
    fog.drift   = ReadClampedFloat(inputStream, 0.0f, 4.0f);
    fog.ambient = ReadClampedFloat(inputStream, 0.0f, 2.0f);
    return fog;
}

float Packet::ReadClampedFloat(DataInputStream *inputStream, float min, float max)
{
    float value = inputStream->ReadFloat();
    return std::isfinite(value) ? std::clamp(value, min, max) : min;
}
