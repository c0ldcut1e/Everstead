#include "ContainerSetContentS2CPacket.h"

#include "../../listener/PacketListener.h"

ContainerSetContentS2CPacket::ContainerSetContentS2CPacket()
{
    m_menuId       = 0;
    m_selectedSlot = 0;
}

ContainerSetContentS2CPacket::ContainerSetContentS2CPacket(uint8_t menuId, uint8_t selectedSlot, const std::vector<ItemInstance> &items,
                                                           const ItemInstance &carried)
{
    m_menuId       = menuId;
    m_selectedSlot = selectedSlot;
    m_items        = items;
    m_carried      = carried;
}

ContainerSetContentS2CPacket::~ContainerSetContentS2CPacket()
{}

ePACKET_ID ContainerSetContentS2CPacket::GetId()
{
    return ePacketId_ContainerSetContentS2C;
}

void ContainerSetContentS2CPacket::Read(DataInputStream *inputStream)
{
    m_menuId       = inputStream->ReadByte();
    m_selectedSlot = inputStream->ReadByte();

    int count = (int) inputStream->ReadByte();
    if (count > s_maxSlots)
    {
        count = s_maxSlots;
    }

    m_items.clear();
    m_items.reserve((size_t) count);
    for (int i = 0; i < count; i++)
    {
        m_items.push_back(ReadItem(inputStream));
    }

    m_carried = ReadItem(inputStream);
}

void ContainerSetContentS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_menuId);
    outputStream->WriteByte(m_selectedSlot);

    size_t count = m_items.size() < (size_t) s_maxSlots ? m_items.size() : (size_t) s_maxSlots;
    outputStream->WriteByte((uint8_t) count);
    for (size_t i = 0; i < count; i++)
    {
        WriteItem(outputStream, m_items[i]);
    }

    WriteItem(outputStream, m_carried);
}

void ContainerSetContentS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerSetContent(this);
}

int ContainerSetContentS2CPacket::GetEstimatedSize()
{
    return 9 + (int) m_items.size() * 5;
}

uint8_t ContainerSetContentS2CPacket::GetMenuId() const
{
    return m_menuId;
}

const ItemInstance &ContainerSetContentS2CPacket::GetCarried() const
{
    return m_carried;
}

uint8_t ContainerSetContentS2CPacket::GetSelectedSlot() const
{
    return m_selectedSlot;
}

const std::vector<ItemInstance> &ContainerSetContentS2CPacket::GetItems() const
{
    return m_items;
}
