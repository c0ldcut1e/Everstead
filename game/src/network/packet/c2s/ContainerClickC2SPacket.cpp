#include "ContainerClickC2SPacket.h"

#include "../../listener/PacketListener.h"

ContainerClickC2SPacket::ContainerClickC2SPacket()
{
    m_menuId = 0;
    m_slot   = 0;
    m_button = 0;
    m_type   = eClickType_Pickup;
}

ContainerClickC2SPacket::ContainerClickC2SPacket(uint8_t menuId, int16_t slot, uint8_t button, eCLICK_TYPE type)
{
    m_menuId = menuId;
    m_slot   = slot;
    m_button = button;
    m_type   = type;
}

ContainerClickC2SPacket::ContainerClickC2SPacket(uint8_t menuId, const std::vector<int> &dragSlots, uint8_t button)
{
    m_menuId    = menuId;
    m_slot      = 0;
    m_button    = button;
    m_type      = eClickType_Drag;
    m_dragSlots = dragSlots;
}

ContainerClickC2SPacket::~ContainerClickC2SPacket()
{}

ePACKET_ID ContainerClickC2SPacket::GetId()
{
    return ePacketId_ContainerClickC2S;
}

void ContainerClickC2SPacket::Read(DataInputStream *inputStream)
{
    m_menuId = inputStream->ReadByte();
    m_slot   = (int16_t) inputStream->ReadShort();
    m_button = inputStream->ReadByte();

    uint8_t type = inputStream->ReadByte();
    m_type       = type < eClickType_Count ? (eCLICK_TYPE) type : eClickType_Count;

    m_dragSlots.clear();
    if (m_type != eClickType_Drag)
    {
        return;
    }

    int count = (int) inputStream->ReadByte();
    if (count > s_maxDragSlots)
    {
        count = s_maxDragSlots;
    }

    m_dragSlots.reserve((size_t) count);
    for (int i = 0; i < count; i++)
    {
        m_dragSlots.push_back((int) (int16_t) inputStream->ReadShort());
    }
}

void ContainerClickC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_menuId);
    outputStream->WriteShort((uint16_t) m_slot);
    outputStream->WriteByte(m_button);
    outputStream->WriteByte((uint8_t) m_type);

    if (m_type != eClickType_Drag)
    {
        return;
    }

    size_t count = m_dragSlots.size() < (size_t) s_maxDragSlots ? m_dragSlots.size() : (size_t) s_maxDragSlots;
    outputStream->WriteByte((uint8_t) count);
    for (size_t i = 0; i < count; i++)
    {
        outputStream->WriteShort((uint16_t) (int16_t) m_dragSlots[i]);
    }
}

void ContainerClickC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerClick(this);
}

int ContainerClickC2SPacket::GetEstimatedSize()
{
    return 6 + (int) m_dragSlots.size() * 2;
}

uint8_t ContainerClickC2SPacket::GetMenuId() const
{
    return m_menuId;
}

int16_t ContainerClickC2SPacket::GetSlot() const
{
    return m_slot;
}

uint8_t ContainerClickC2SPacket::GetButton() const
{
    return m_button;
}

eCLICK_TYPE ContainerClickC2SPacket::GetType() const
{
    return m_type;
}

const std::vector<int> &ContainerClickC2SPacket::GetDragSlots() const
{
    return m_dragSlots;
}
