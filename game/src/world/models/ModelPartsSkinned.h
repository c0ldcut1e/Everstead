#pragma once

#include <string>

#include "ModelDefinition.h"
#include "PlayerSkinModel.h"

class ModelPartsSkinned
{
public:
    static ModelDefinition CreateSteveClassic(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSteveSlim(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreatePlayer(const std::string &id, const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel);
    static ModelDefinition CreateHumanoid(const std::string &id, const std::string &texturePath, int textureHeight, float inflate);
    static ModelDefinition CreateSkeleton(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateCape(const std::string &id, const std::string &texturePath);
};
