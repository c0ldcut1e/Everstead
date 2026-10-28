#include "ComparatorBlock.h"

#include "../Level.h"

ComparatorBlock::ComparatorBlock() : DiodeBlock("assets/texture/block/comparator.png", "assets/texture/block/comparator_on.png")
{
    m_modeProperty   = AddProperty("mode", 2);
    m_outputProperty = AddProperty("output", 16);
}

void ComparatorBlock::Tick(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    int output    = CalculateOutput(level, pos, data);
    int previous  = GetOutputSignal(data);
    uint16_t next = WithPropertyValue(data, m_outputProperty, output);
    if (previous != output || !IsSubtract(data))
    {
        next = WithPropertyValue(next, m_poweredProperty, ShouldTurnOn(level, pos, data) ? 1 : 0);
    }

    if (next == data)
    {
        return;
    }

    SetData(level, pos, next);
    if (previous != output || !IsSubtract(data))
    {
        UpdateNeighborsInFront(level, pos, next);
    }
}

bool ComparatorBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    bool subtract = !IsSubtract(data);
    SetData(level, pos, WithPropertyValue(data, m_modeProperty, subtract ? 1 : 0));
    level->PlaySound("random.click", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.3f, subtract ? 0.55f : 0.5f);
    Tick(level, pos);
    return true;
}

void ComparatorBlock::GetTorches(uint16_t data, std::vector<glm::dvec3> *out, std::vector<bool> *lit) const
{
    Direction *output = GetOutput(data);
    out->push_back(ToWorld(output, glm::dvec3(5.0 / 16.0, -3.0 / 16.0, 12.0 / 16.0)));
    out->push_back(ToWorld(output, glm::dvec3(11.0 / 16.0, -3.0 / 16.0, 12.0 / 16.0)));
    out->push_back(ToWorld(output, glm::dvec3(0.5, -5.0 / 16.0, 3.0 / 16.0)));
    lit->push_back(IsPowered(data));
    lit->push_back(IsPowered(data));
    lit->push_back(IsSubtract(data));
}

void ComparatorBlock::GetElements(uint16_t data, std::vector<Element> *out) const
{
    AddBase(out, IsPowered(data));
}

void ComparatorBlock::CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data)
{
    if (CalculateOutput(level, pos, data) != GetOutputSignal(data) || IsPowered(data) != ShouldTurnOn(level, pos, data))
    {
        level->ScheduleBlockForTick(pos, 2, IsFacingTowardsDiode(level, pos, data) ? -1 : 0);
    }
}

bool ComparatorBlock::ShouldTurnOn(Level *level, const BlockPos &pos, uint16_t data) const
{
    int input = GetInputSignal(level, pos, data);
    if (input == 0)
    {
        return false;
    }

    int side = GetAlternateSignal(level, pos, data);
    return input > side || (input == side && !IsSubtract(data));
}

int ComparatorBlock::GetDelay(uint16_t data) const
{
    (void) data;
    return 2;
}

int ComparatorBlock::GetOutputSignal(uint16_t data) const
{
    return GetPropertyValue(data, m_outputProperty);
}

int ComparatorBlock::GetInputSignal(Level *level, const BlockPos &pos, uint16_t data) const
{
    int signal        = DiodeBlock::GetInputSignal(level, pos, data);
    Direction *output = GetOutput(data);
    BlockPos input(pos.x - output->dx, pos.y, pos.z - output->dz);
    Block *block = Block::ById(level->GetBlockId(input));
    if (block->HasAnalogOutput())
    {
        return block->GetAnalogOutput(level, input);
    }

    if (signal < 15 && block->IsSignalConductor())
    {
        BlockPos behind(input.x - output->dx, input.y, input.z - output->dz);
        Block *behindBlock = Block::ById(level->GetBlockId(behind));
        if (behindBlock->HasAnalogOutput())
        {
            return behindBlock->GetAnalogOutput(level, behind);
        }
    }

    return signal;
}

bool ComparatorBlock::IsSubtract(uint16_t data) const
{
    return GetPropertyValue(data, m_modeProperty) == 1;
}

int ComparatorBlock::CalculateOutput(Level *level, const BlockPos &pos, uint16_t data) const
{
    int input = GetInputSignal(level, pos, data);
    if (input == 0)
    {
        return 0;
    }

    int side = GetAlternateSignal(level, pos, data);
    if (side > input)
    {
        return 0;
    }

    return IsSubtract(data) ? input - side : input;
}
