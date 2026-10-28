#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelRegistry.h"

#include "../../core/Logger.h"
#include "../../entity/animal/Parrot.h"
#include "../../entity/item/Boat.h"
#include "Model.h"
#include "ModelPartsAnimal.h"
#include "ModelPartsSkinned.h"

ModelRegistry *ModelRegistry::GetInstance()
{
    static ModelRegistry instance;
    return &instance;
}

void ModelRegistry::Init()
{
    ModelRegistry *registry = ModelRegistry::GetInstance();

    ModelDefinition steveDef     = ModelPartsSkinned::CreateSteveClassic("steve", "assets/texture/models/steve.png");
    std::unique_ptr<Model> steve = std::make_unique<Model>(steveDef);
    registry->RegisterModel("steve", std::move(steve));

    ModelDefinition slimSteveDef     = ModelPartsSkinned::CreateSteveSlim("steve_slim", "assets/texture/models/steve.png");
    std::unique_ptr<Model> slimSteve = std::make_unique<Model>(slimSteveDef);
    registry->RegisterModel("steve_slim", std::move(slimSteve));

    registry->RegisterModel("armor_outer", std::make_unique<Model>(ModelPartsSkinned::CreateHumanoid("armor_outer", "", 32, 1.0f)));
    registry->RegisterModel("armor_inner", std::make_unique<Model>(ModelPartsSkinned::CreateHumanoid("armor_inner", "", 32, 0.5f)));
    registry->RegisterModel("pig", std::make_unique<Model>(ModelPartsAnimal::CreatePig("pig", "assets/texture/entity/pig/pig.png")));
    registry->RegisterModel("cow", std::make_unique<Model>(ModelPartsAnimal::CreateCow("cow", "assets/texture/entity/cow/cow.png")));
    registry->RegisterModel("sheep", std::make_unique<Model>(ModelPartsAnimal::CreateSheep("sheep", "assets/texture/entity/sheep/sheep.png")));
    registry->RegisterModel("sheep_fur",
                            std::make_unique<Model>(ModelPartsAnimal::CreateSheepFur("sheep_fur", "assets/texture/entity/sheep/sheep_fur.png")));
    for (int i = 0; i < Boat::kWoodTypeCount; i++)
    {
        std::string name = std::string("boat_") + Boat::kWoodTypes[i];
        registry->RegisterModel(name, std::make_unique<Model>(ModelPartsAnimal::CreateBoat(name, std::string("assets/texture/entity/boat/") +
                                                                                                         Boat::kWoodTypes[i] + ".png")));
    }

    registry->RegisterModel("boat_water_patch", std::make_unique<Model>(ModelPartsAnimal::CreateBoatWaterPatch(
                                                        "boat_water_patch", "assets/texture/entity/boat/oak.png")));

    registry->RegisterModel("minecart", std::make_unique<Model>(ModelPartsAnimal::CreateMinecart("minecart", "assets/texture/entity/minecart.png")));
    registry->RegisterModel("leash_knot",
                            std::make_unique<Model>(ModelPartsAnimal::CreateLeashKnot("leash_knot", "assets/texture/entity/lead_knot.png")));
    registry->RegisterModel("chicken",
                            std::make_unique<Model>(ModelPartsAnimal::CreateChicken("chicken", "assets/texture/entity/chicken/chicken.png")));
    registry->RegisterModel("ocelot", std::make_unique<Model>(ModelPartsAnimal::CreateOcelot("ocelot", "assets/texture/entity/ocelot/ocelot.png")));
    registry->RegisterModel("squid", std::make_unique<Model>(ModelPartsAnimal::CreateSquid("squid", "assets/texture/entity/squid/squid.png")));
    for (int i = 0; i < Parrot::kVariantCount; i++)
    {
        std::string name = std::string("parrot_") + Parrot::kVariants[i];
        registry->RegisterModel(name, std::make_unique<Model>(ModelPartsAnimal::CreateParrot(name, "assets/texture/entity/parrot/" + name + ".png")));
    }

    registry->RegisterModel(
            "zombie", std::make_unique<Model>(ModelPartsSkinned::CreateHumanoid("zombie", "assets/texture/entity/zombie/zombie.png", 64, 0.0f)));
    registry->RegisterModel("skeleton",
                            std::make_unique<Model>(ModelPartsSkinned::CreateSkeleton("skeleton", "assets/texture/entity/skeleton/skeleton.png")));
    registry->RegisterModel("spider", std::make_unique<Model>(ModelPartsAnimal::CreateSpider("spider", "assets/texture/entity/spider/spider.png")));
    registry->RegisterModel("spider_eyes",
                            std::make_unique<Model>(ModelPartsAnimal::CreateSpider("spider_eyes", "assets/texture/entity/spider/spider_eyes.png")));
    registry->RegisterModel("creeper",
                            std::make_unique<Model>(ModelPartsAnimal::CreateCreeper("creeper", "assets/texture/entity/creeper/creeper.png")));
    registry->RegisterModel("slime", std::make_unique<Model>(ModelPartsAnimal::CreateSlime("slime", "assets/texture/entity/slime/slime.png")));
    registry->RegisterModel("slime_outer",
                            std::make_unique<Model>(ModelPartsAnimal::CreateSlimeOuter("slime_outer", "assets/texture/entity/slime/slime.png")));

    Logger::LogInfo("ModelRegistry initialized (models=%d)", (int) registry->m_models.size());
}

void ModelRegistry::RegisterModel(const std::string &name, std::unique_ptr<Model> model)
{
    if (!model)
    {
        Logger::LogWarn("ModelRegistry: tried to register null model '%s'", name.c_str());
        return;
    }

    if (HasModel(name))
    {
        Logger::LogWarn("ModelRegistry: overwriting model '%s'", name.c_str());
    }

    m_models[name] = std::move(model);
}

Model *ModelRegistry::GetModel(const std::string &name) const
{
    auto it = m_models.find(name);
    if (it == m_models.end())
    {
        return nullptr;
    }

    return it->second.get();
}

bool ModelRegistry::HasModel(const std::string &name) const
{
    return m_models.find(name) != m_models.end();
}

TextureRepository *ModelRegistry::GetTextures()
{
    return TextureRepository::GetInstance();
}

#endif
