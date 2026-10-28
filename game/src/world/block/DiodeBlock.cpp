#include "DiodeBlock.h"

#include <algorithm>
#include <cmath>

#include "../Level.h"
#include "RedstoneWireBlock.h"

#include <glm/common.hpp>
#include <glm/vector_relational.hpp>

static Direction *const s_diodeOutputs[] = {Direction::north, Direction::south, Direction::west, Direction::east};

DiodeBlock::DiodeBlock(const std::string &topPath, const std::string &litTopPath) : Block(false, topPath)
{
    m_facingProperty  = AddProperty("facing", 4);
    m_poweredProperty = AddProperty("powered", 2);
    m_topTexture      = AddTextureVariant(topPath);
    AddTextureVariant(topPath + "#rotated");
    m_litTopTexture = AddTextureVariant(litTopPath);
    AddTextureVariant(litTopPath + "#rotated");
    m_slabTexture = AddTextureVariant("assets/texture/block/smooth_stone.png");
    AddTextureVariant("assets/texture/block/smooth_stone.png#rotated");
    m_torchTexture = AddTextureVariant("assets/texture/block/redstone_torch.png");
    AddTextureVariant("assets/texture/block/redstone_torch.png#rotated");
    m_unlitTorchTexture = AddTextureVariant("assets/texture/block/redstone_torch_off.png");
    AddTextureVariant("assets/texture/block/redstone_torch_off.png#rotated");
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetFlatItem(true);
    SetBreakSound("dig.wood");
}

void DiodeBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    UpdateNeighborsInFront(level, pos, data);
    if (ShouldTurnOn(level, pos, data))
    {
        level->ScheduleBlockForTick(pos, 1, 0);
    }
}

void DiodeBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (!level->IsClientSide())
    {
        UpdateNeighborsInFront(level, pos, data);
    }
}

void DiodeBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    CheckTickOnNeighbor(level, pos, level->GetBlockData(pos));
}

void DiodeBlock::CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data)
{
    if (IsLocked(level, pos, data))
    {
        return;
    }

    bool powered = IsPowered(data);
    if (powered == ShouldTurnOn(level, pos, data))
    {
        return;
    }

    int priority = -1;
    if (IsFacingTowardsDiode(level, pos, data))
    {
        priority = -3;
    }
    else if (powered)
    {
        priority = -2;
    }

    level->ScheduleBlockForTick(pos, GetDelay(data), priority);
}

void DiodeBlock::Tick(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (IsLocked(level, pos, data))
    {
        return;
    }

    bool powered = IsPowered(data);
    bool should  = ShouldTurnOn(level, pos, data);
    if (powered && !should)
    {
        SetData(level, pos, WithPropertyValue(data, m_poweredProperty, 0));
        UpdateNeighborsInFront(level, pos, data);
    }
    else if (!powered)
    {
        SetData(level, pos, WithPropertyValue(data, m_poweredProperty, 1));
        UpdateNeighborsInFront(level, pos, data);
        if (!should)
        {
            level->ScheduleBlockForTick(pos, GetDelay(data), -1);
        }
    }
}

void DiodeBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0, 2.0 / 16.0, 1.0)));
}

void DiodeBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    std::vector<Element> elements;
    GetElements(data, &elements);
    for (size_t i = 0; i < elements.size(); i++)
    {
        out->push_back(ToWorldBox(GetOutput(data), elements[i]));
    }
}

Block::UVRect DiodeBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU      = false;
    int texture = 0;
    float u0    = 0.0f;
    float v0    = 0.0f;
    float u1    = 0.0f;
    float v1    = 0.0f;
    GetFaceUV(data, box, direction, &texture, &u0, &v0, &u1, &v1);
    return GetTextureVariantAtlasUVRect(texture);
}

void DiodeBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    int texture = 0;
    GetFaceUV(data, box, direction, &texture, u0, v0, u1, v1);
}

uint16_t DiodeBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (fabs(look.x) > fabs(look.z))
    {
        facing = look.x > 0.0 ? 3 : 2;
    }
    else
    {
        facing = look.z > 0.0 ? 1 : 0;
    }

    return WithPropertyValue(0, m_facingProperty, facing);
}

bool DiodeBlock::IsUsable(uint16_t data) const
{
    (void) data;
    return true;
}

int DiodeBlock::GetSignal(uint16_t data, Direction *direction) const
{
    return GetDirectSignal(data, direction);
}

int DiodeBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return IsPowered(data) && direction == GetOutput(data) ? GetOutputSignal(data) : 0;
}

bool DiodeBlock::IsSignalSource() const
{
    return true;
}

Direction *DiodeBlock::GetOutput(uint16_t data) const
{
    return s_diodeOutputs[GetPropertyValue(data, m_facingProperty)];
}

bool DiodeBlock::IsPowered(uint16_t data) const
{
    return GetPropertyValue(data, m_poweredProperty) == 1;
}

Block::UVRect DiodeBlock::GetTorchAtlasUVRect(bool lit) const
{
    return GetTextureVariantAtlasUVRect(lit ? m_torchTexture : m_unlitTorchTexture);
}

bool DiodeBlock::IsLocked(Level *level, const BlockPos &pos, uint16_t data) const
{
    (void) level;
    (void) pos;
    (void) data;
    return false;
}

bool DiodeBlock::ShouldTurnOn(Level *level, const BlockPos &pos, uint16_t data) const
{
    return GetInputSignal(level, pos, data) > 0;
}

bool DiodeBlock::IsAlternateInput(Block *block) const
{
    return block->IsSignalSource();
}

int DiodeBlock::GetInputSignal(Level *level, const BlockPos &pos, uint16_t data) const
{
    Direction *output = GetOutput(data);
    BlockPos input(pos.x - output->dx, pos.y, pos.z - output->dz);
    int signal = level->GetSignal(input, output);
    if (signal >= 15)
    {
        return signal;
    }

    RedstoneWireBlock *wire = (RedstoneWireBlock *) Block::ByName("redstone_wire");
    if (Block::ById(level->GetBlockId(input)) != wire)
    {
        return signal;
    }

    return std::max(signal, wire->GetWirePower(level, input));
}

int DiodeBlock::GetAlternateSignal(Level *level, const BlockPos &pos, uint16_t data) const
{
    Direction *output = GetOutput(data);
    Direction *sides[2];
    sides[0]   = output->dx != 0 ? Direction::north : Direction::west;
    sides[1]   = output->dx != 0 ? Direction::south : Direction::east;
    int signal = 0;
    for (int i = 0; i < 2; i++)
    {
        BlockPos side(pos.x + sides[i]->dx, pos.y, pos.z + sides[i]->dz);
        Block *block = Block::ById(level->GetBlockId(side));
        if (!IsAlternateInput(block))
        {
            continue;
        }

        if (block == Block::ByName("redstone_block"))
        {
            signal = std::max(signal, 15);
        }
        else if (block == Block::ByName("redstone_wire"))
        {
            signal = std::max(signal, ((RedstoneWireBlock *) block)->GetWirePower(level, side));
        }
        else
        {
            signal = std::max(signal, block->GetDirectSignal(level->GetBlockData(side), Direction::Opposite(sides[i])));
        }
    }

    return signal;
}

bool DiodeBlock::IsFacingTowardsDiode(Level *level, const BlockPos &pos, uint16_t data) const
{
    Direction *output = GetOutput(data);
    BlockPos front(pos.x + output->dx, pos.y, pos.z + output->dz);
    DiodeBlock *diode = dynamic_cast<DiodeBlock *>(Block::ById(level->GetBlockId(front)));
    return diode && diode->GetOutput(level->GetBlockData(front)) != Direction::Opposite(output);
}

void DiodeBlock::UpdateNeighborsInFront(Level *level, const BlockPos &pos, uint16_t data) const
{
    Direction *output = GetOutput(data);
    level->NotifyBlockAndNeighbors(BlockPos(pos.x + output->dx, pos.y, pos.z + output->dz), pos);
}

void DiodeBlock::SetData(Level *level, const BlockPos &pos, uint16_t data) const
{
    level->SetBlockData(pos, data);
    level->RecordBlockChange(pos);
}

void DiodeBlock::AddBase(std::vector<Element> *out, bool lit) const
{
    Element element;
    element.from = glm::vec3(0.0f);
    element.to   = glm::vec3(16.0f, 2.0f, 16.0f);
    for (int i = 0; i < 4; i++)
    {
        element.faces[i] = Face{m_slabTexture, glm::vec4(0.0f, 14.0f, 16.0f, 16.0f), 0};
    }

    element.faces[4] = Face{m_slabTexture, glm::vec4(0.0f, 0.0f, 16.0f, 16.0f), 0};
    element.faces[5] = Face{lit ? m_litTopTexture : m_topTexture, glm::vec4(0.0f, 0.0f, 16.0f, 16.0f), 0};
    out->push_back(element);
}

void DiodeBlock::AddElement(std::vector<Element> *out, const glm::vec3 &from, const glm::vec3 &to, int texture, const glm::vec4 &northSouthUV,
                            const glm::vec4 &westEastUV, const glm::vec4 &upDownUV, int upDownRotation) const
{
    Element element;
    element.from     = from;
    element.to       = to;
    element.faces[0] = Face{texture, northSouthUV, 0};
    element.faces[1] = Face{texture, northSouthUV, 0};
    element.faces[2] = Face{texture, westEastUV, 0};
    element.faces[3] = Face{texture, westEastUV, 0};
    element.faces[4] = Face{texture, upDownUV, upDownRotation};
    element.faces[5] = Face{texture, upDownUV, upDownRotation};
    out->push_back(element);
}

// Models are built with the output on the north side and turned to face the real output
glm::dvec3 DiodeBlock::ToWorld(Direction *output, const glm::dvec3 &model) const
{
    if (output == Direction::north)
    {
        return model;
    }

    if (output == Direction::east)
    {
        return glm::dvec3(1.0 - model.z, model.y, model.x);
    }

    if (output == Direction::south)
    {
        return glm::dvec3(1.0 - model.x, model.y, 1.0 - model.z);
    }

    return glm::dvec3(model.z, model.y, 1.0 - model.x);
}

glm::dvec3 DiodeBlock::ToModel(Direction *output, const glm::dvec3 &world) const
{
    if (output == Direction::north)
    {
        return world;
    }

    if (output == Direction::east)
    {
        return glm::dvec3(world.z, world.y, 1.0 - world.x);
    }

    if (output == Direction::south)
    {
        return glm::dvec3(1.0 - world.x, world.y, 1.0 - world.z);
    }

    return glm::dvec3(1.0 - world.z, world.y, world.x);
}

AABB DiodeBlock::ToWorldBox(Direction *output, const Element &element) const
{
    glm::dvec3 a = ToWorld(output, glm::dvec3(element.from) / 16.0);
    glm::dvec3 b = ToWorld(output, glm::dvec3(element.to) / 16.0);
    return AABB(glm::min(a, b), glm::max(a, b));
}

void DiodeBlock::GetFaceUV(uint16_t data, const AABB &box, Direction *direction, int *texture, float *u0, float *v0, float *u1, float *v1) const
{
    Direction *output = GetOutput(data);
    std::vector<Element> elements;
    GetElements(data, &elements);
    const Element *element = nullptr;
    for (size_t i = 0; i < elements.size(); i++)
    {
        AABB world = ToWorldBox(output, elements[i]);
        if (glm::all(glm::lessThan(glm::abs(world.GetMin() - box.GetMin()), glm::dvec3(1e-4))) &&
            glm::all(glm::lessThan(glm::abs(world.GetMax() - box.GetMax()), glm::dvec3(1e-4))))
        {
            element = &elements[i];
            break;
        }
    }

    glm::dvec3 normal = glm::round(ToModel(output, glm::dvec3(0.5) + glm::dvec3(direction->dx, direction->dy, direction->dz) * 0.5) - 0.5);
    Direction *face   = direction;
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *candidate = Direction::ById(i);
        if (candidate->dx == (int) normal.x && candidate->dy == (int) normal.y && candidate->dz == (int) normal.z)
        {
            face = candidate;
        }
    }

    glm::dvec3 corners[3];
    GetBoxFaceCorners(box, direction, corners);

    const Face &modelFace = element->faces[face->GetId() - 1];
    glm::vec2 from        = GetModelFaceAxes(face, element->from / 16.0f);
    glm::vec2 to          = GetModelFaceAxes(face, element->to / 16.0f);
    glm::vec2 low         = glm::min(from, to);
    glm::vec2 size        = glm::abs(to - from);
    glm::vec2 st[3];
    for (int i = 0; i < 3; i++)
    {
        glm::vec2 f = (GetModelFaceAxes(face, glm::vec3(ToModel(output, corners[i]))) - low) / size;
        if (modelFace.rotation == 90)
        {
            f = glm::vec2(f.y, 1.0f - f.x);
        }
        else if (modelFace.rotation == 180)
        {
            f = glm::vec2(1.0f - f.x, 1.0f - f.y);
        }
        else if (modelFace.rotation == 270)
        {
            f = glm::vec2(1.0f - f.y, f.x);
        }

        st[i] = glm::vec2(modelFace.uv.x + (modelFace.uv.z - modelFace.uv.x) * f.x, modelFace.uv.y + (modelFace.uv.w - modelFace.uv.y) * f.y) / 16.0f;
    }

    ResolveFaceUV(modelFace.texture, st, texture, u0, v0, u1, v1);
}
