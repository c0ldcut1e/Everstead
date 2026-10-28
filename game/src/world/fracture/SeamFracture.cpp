#include "SeamFracture.h"

#include <vector>

#include "../../entity/player/ServerPlayer.h"
#include "../../entity/player/ServerPlayerGameMode.h"
#include "../Level.h"
#include "../block/Block.h"
#include "FractureTraits.h"

SeamFracture::SeamFracture() : FractureTrait("seam", FractureTraits::eGearType_Tool, 1)
{}

int SeamFracture::OnHarvest(ServerPlayer *player, const Harvest &harvest)
{
    if (harvest.cascade || !harvest.block->GetResourceLocation().GetPath().ends_with("_ore") || harvest.level->GetRandom().NextFloat() >= 0.75f)
    {
        return 0;
    }

    std::vector<BlockPos> vein = {harvest.pos};
    for (size_t i = 0; i < vein.size() && vein.size() <= 32; i++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dz = -1; dz <= 1; dz++)
                {
                    BlockPos next(vein[i].x + dx, vein[i].y + dy, vein[i].z + dz);
                    if (vein.size() <= 32 && Block::ById(harvest.level->GetBlockId(next)) == harvest.block &&
                        player->GetGameMode()->BreakExtraBlock(harvest.level, next, harvest.held))
                    {
                        vein.push_back(next);
                    }
                }
            }
        }
    }

    if (vein.size() > 1)
    {
        PlaySound(harvest.level, glm::dvec3(harvest.pos.x + 0.5, harvest.pos.y + 0.5, harvest.pos.z + 0.5), 1.0f);
    }

    return (int) vein.size() - 1;
}
