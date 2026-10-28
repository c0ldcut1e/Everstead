#pragma once

#include <string>

#include "ModelDefinition.h"

class ModelPartsAnimal
{
public:
    static ModelDefinition CreateQuadruped(const std::string &id, const std::string &texturePath, int legSize);
    static ModelDefinition CreatePig(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateCow(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSheep(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSheepFur(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateChicken(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSquid(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateOcelot(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateParrot(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSpider(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateCreeper(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSlime(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateSlimeOuter(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateBoat(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateBoatWaterPatch(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateMinecart(const std::string &id, const std::string &texturePath);
    static ModelDefinition CreateLeashKnot(const std::string &id, const std::string &texturePath);

private:
    static ModelPartCubeDefinition Box(const std::string &name, int u, int v, float x, float y, float z, int width, int height, int depth,
                                       float grow = 0.0f);
};
