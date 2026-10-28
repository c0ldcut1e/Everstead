#include "TntBlock.h"

#include <memory>

#include "../../entity/item/PrimedTnt.h"
#include "../Level.h"

TntBlock::TntBlock() : Block(true, "assets/texture/block/tnt_side.png")
{
    SetFaceTexture(Direction::up, "assets/texture/block/tnt_top.png");
    SetFaceTexture(Direction::down, "assets/texture/block/tnt_bottom.png");
    SetBreakSound("dig.grass");
    SetFlammable(15, 100);
}

// We ignite on the next tick because replacing the block inside OnPlace would run before its own placement finishes
void TntBlock::OnPlace(Level *level, const BlockPos &pos)
{
    Block::OnPlace(level, pos);
    if (!level->IsClientSide() && level->GetBestNeighborSignal(pos) > 0)
    {
        level->ScheduleBlockForTick(pos, 1, 0);
    }
}

void TntBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    IgniteIfPowered(level, pos);
}

void TntBlock::Tick(Level *level, const BlockPos &pos)
{
    IgniteIfPowered(level, pos);
}

void TntBlock::IgniteIfPowered(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || level->GetBestNeighborSignal(pos) == 0)
    {
        return;
    }

    Ignite(level, pos);
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

void TntBlock::OnExploded(Level *level, const BlockPos &pos)
{
    std::unique_ptr<PrimedTnt> primed(new PrimedTnt(level));
    primed->Ignite(glm::dvec3(pos.x + 0.5, pos.y, pos.z + 0.5), level->GetRandom().NextInt(0, 19) + 10);
    level->AddEntity(std::move(primed));
}

bool TntBlock::DropsFromExplosion() const
{
    return false;
}

// The caller removes the block
void TntBlock::Ignite(Level *level, const BlockPos &pos)
{
    std::unique_ptr<PrimedTnt> primed(new PrimedTnt(level));
    primed->Ignite(glm::dvec3(pos.x + 0.5, pos.y, pos.z + 0.5), 80);
    level->PlaySound("random.fuse", primed->GetPosition(), 1.0f, 1.0f);
    level->AddEntity(std::move(primed));
}
