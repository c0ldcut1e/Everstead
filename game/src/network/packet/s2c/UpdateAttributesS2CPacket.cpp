#include "UpdateAttributesS2CPacket.h"

#include <algorithm>

#include "../../listener/PacketListener.h"

UpdateAttributesS2CPacket::UpdateAttributesS2CPacket()
{
    m_entries = {};
}

UpdateAttributesS2CPacket::UpdateAttributesS2CPacket(const AttributeMap &attributes)
{
    const std::vector<AttributeInstance> &instances = attributes.GetInstances();
    for (size_t i = 0; i < instances.size(); i++)
    {
        m_entries.push_back(
                {instances[i].GetAttribute()->GetResourceLocation().ToString(), instances[i].GetBaseValue(), instances[i].GetModifiers()});
    }
}

UpdateAttributesS2CPacket::~UpdateAttributesS2CPacket()
{}

ePACKET_ID UpdateAttributesS2CPacket::GetId()
{
    return ePacketId_UpdateAttributesS2C;
}

void UpdateAttributesS2CPacket::Read(DataInputStream *inputStream)
{
    uint16_t entryCount = std::min(inputStream->ReadShort(), kMaxEntries);
    m_entries.resize(entryCount);
    for (uint16_t i = 0; i < entryCount; i++)
    {
        m_entries[i].name      = inputStream->ReadString();
        m_entries[i].baseValue = inputStream->ReadDouble();
        uint16_t modifierCount = std::min(inputStream->ReadShort(), kMaxModifiers);
        m_entries[i].modifiers.resize(modifierCount);
        for (uint16_t j = 0; j < modifierCount; j++)
        {
            m_entries[i].modifiers[j].id     = inputStream->ReadString();
            m_entries[i].modifiers[j].amount = inputStream->ReadDouble();
            m_entries[i].modifiers[j].operation =
                    (eATTRIBUTE_OPERATION) std::min(inputStream->ReadByte(), (uint8_t) eAttributeOperation_AddMultipliedTotal);
        }
    }
}

void UpdateAttributesS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteShort((uint16_t) m_entries.size());
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        outputStream->WriteString(m_entries[i].name);
        outputStream->WriteDouble(m_entries[i].baseValue);
        outputStream->WriteShort((uint16_t) m_entries[i].modifiers.size());
        for (size_t j = 0; j < m_entries[i].modifiers.size(); j++)
        {
            outputStream->WriteString(m_entries[i].modifiers[j].id);
            outputStream->WriteDouble(m_entries[i].modifiers[j].amount);
            outputStream->WriteByte((uint8_t) m_entries[i].modifiers[j].operation);
        }
    }
}

void UpdateAttributesS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleUpdateAttributes(this);
}

int UpdateAttributesS2CPacket::GetEstimatedSize()
{
    int size = 2;
    for (size_t i = 0; i < m_entries.size(); i++)
    {
        size += 14 + (int) m_entries[i].name.size() + (int) m_entries[i].modifiers.size() * 20;
    }

    return size;
}

const std::vector<UpdateAttributesS2CPacket::Entry> &UpdateAttributesS2CPacket::GetEntries() const
{
    return m_entries;
}
