#pragma once

#include <string>
#include <vector>

#include "../../../entity/attribute/AttributeMap.h"
#include "../Packet.h"

class UpdateAttributesS2CPacket : public Packet
{
public:
    struct Entry
    {
        std::string name;
        double baseValue;
        std::vector<AttributeModifier> modifiers;
    };

    UpdateAttributesS2CPacket();
    explicit UpdateAttributesS2CPacket(const AttributeMap &attributes);
    ~UpdateAttributesS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::vector<Entry> &GetEntries() const;

private:
    static constexpr uint16_t kMaxEntries   = 256;
    static constexpr uint16_t kMaxModifiers = 256;

    std::vector<Entry> m_entries;
};
