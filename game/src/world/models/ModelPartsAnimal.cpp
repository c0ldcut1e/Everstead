#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPartsAnimal.h"

#include <cmath>
#include <memory>
#include <numbers>
#include <string>

ModelDefinition ModelPartsAnimal::CreateQuadruped(const std::string &id, const std::string &texturePath, int legSize)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 6.0 + legSize, 6.0));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 7.0 + legSize, -2.0));
    body->SetRotation(glm::dvec3(90.0, 0.0, 0.0));

    const char *kLegNames[4] = {"leg0", "leg1", "leg2", "leg3"};
    const double kLegX[4]    = {-3.0, 3.0, -3.0, 3.0};
    const double kLegZ[4]    = {-7.0, -7.0, 5.0, 5.0};
    for (int i = 0; i < 4; i++)
    {
        std::unique_ptr<ModelPartDefinition> leg = std::make_unique<ModelPartDefinition>(kLegNames[i]);
        leg->SetPosition(glm::dvec3(kLegX[i], legSize, kLegZ[i]));
        leg->AddCube(Box(std::string(kLegNames[i]) + "_cube", 0, 16, -2.0f, 0.0f, -2.0f, 4, legSize, 4));
        root->AddChild(std::move(leg));
    }

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreatePig(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition = CreateQuadruped(id, texturePath, 6);
    ModelPartDefinition *head  = definition.GetRoot()->GetChild("head");
    head->AddCube(Box("head_cube", 0, 0, -4.0f, -4.0f, -8.0f, 8, 8, 8));
    head->AddCube(Box("snout_cube", 16, 16, -2.0f, 0.0f, -9.0f, 4, 3, 1));
    definition.GetRoot()->GetChild("body")->AddCube(Box("body_cube", 28, 8, -5.0f, -10.0f, -7.0f, 10, 16, 8));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateCow(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition = CreateQuadruped(id, texturePath, 12);
    ModelPartDefinition *root  = definition.GetRoot();
    ModelPartDefinition *head  = root->GetChild("head");
    head->SetPosition(glm::dvec3(0.0, 20.0, 8.0));
    head->AddCube(Box("head_cube", 0, 0, -4.0f, -4.0f, -6.0f, 8, 8, 6));
    head->AddCube(Box("horn0_cube", 22, 0, -5.0f, -5.0f, -4.0f, 1, 3, 1));
    head->AddCube(Box("horn1_cube", 22, 0, 4.0f, -5.0f, -4.0f, 1, 3, 1));

    ModelPartDefinition *body = root->GetChild("body");
    body->SetPosition(glm::dvec3(0.0, 19.0, -2.0));
    body->AddCube(Box("body_cube", 18, 4, -6.0f, -10.0f, -7.0f, 12, 18, 10));
    body->AddCube(Box("udder_cube", 52, 0, -2.0f, 2.0f, -8.0f, 4, 6, 1));

    root->GetChild("leg0")->SetPosition(glm::dvec3(-4.0, 12.0, -7.0));
    root->GetChild("leg1")->SetPosition(glm::dvec3(4.0, 12.0, -7.0));
    root->GetChild("leg2")->SetPosition(glm::dvec3(-4.0, 12.0, 6.0));
    root->GetChild("leg3")->SetPosition(glm::dvec3(4.0, 12.0, 6.0));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSheep(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition = CreateQuadruped(id, texturePath, 12);
    ModelPartDefinition *head  = definition.GetRoot()->GetChild("head");
    head->SetPosition(glm::dvec3(0.0, 18.0, 8.0));
    head->AddCube(Box("head_cube", 0, 0, -3.0f, -4.0f, -6.0f, 6, 6, 8));

    ModelPartDefinition *body = definition.GetRoot()->GetChild("body");
    body->SetPosition(glm::dvec3(0.0, 19.0, -2.0));
    body->AddCube(Box("body_cube", 28, 8, -4.0f, -10.0f, -7.0f, 8, 16, 6));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSheepFur(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 18.0, 8.0));
    head->AddCube(Box("head_cube", 0, 0, -3.0f, -4.0f, -4.0f, 6, 6, 6, 0.6f));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 19.0, -2.0));
    body->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    body->AddCube(Box("body_cube", 28, 8, -4.0f, -10.0f, -7.0f, 8, 16, 6, 1.75f));

    const char *kLegNames[4] = {"leg0", "leg1", "leg2", "leg3"};
    const double kLegX[4]    = {-3.0, 3.0, -3.0, 3.0};
    const double kLegZ[4]    = {-7.0, -7.0, 5.0, 5.0};
    for (int i = 0; i < 4; i++)
    {
        std::unique_ptr<ModelPartDefinition> leg = std::make_unique<ModelPartDefinition>(kLegNames[i]);
        leg->SetPosition(glm::dvec3(kLegX[i], 12.0, kLegZ[i]));
        leg->AddCube(Box(std::string(kLegNames[i]) + "_cube", 0, 16, -2.0f, 0.0f, -2.0f, 4, 6, 4, 0.5f));
        root->AddChild(std::move(leg));
    }

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateChicken(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 9.0, 4.0));
    head->AddCube(Box("head_cube", 0, 0, -2.0f, -6.0f, -2.0f, 4, 6, 3));
    head->AddCube(Box("beak_cube", 14, 0, -2.0f, -4.0f, -4.0f, 4, 2, 2));
    head->AddCube(Box("wattle_cube", 14, 4, -1.0f, -2.0f, -3.0f, 2, 2, 2));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 8.0, 0.0));
    body->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    body->AddCube(Box("body_cube", 0, 9, -3.0f, -4.0f, -3.0f, 6, 8, 6));

    std::unique_ptr<ModelPartDefinition> leg0 = std::make_unique<ModelPartDefinition>("leg0");
    leg0->SetPosition(glm::dvec3(-2.0, 5.0, -1.0));
    leg0->AddCube(Box("leg0_cube", 26, 0, -1.0f, 0.0f, -3.0f, 3, 5, 3));

    std::unique_ptr<ModelPartDefinition> leg1 = std::make_unique<ModelPartDefinition>("leg1");
    leg1->SetPosition(glm::dvec3(1.0, 5.0, -1.0));
    leg1->AddCube(Box("leg1_cube", 26, 0, -1.0f, 0.0f, -3.0f, 3, 5, 3));

    std::unique_ptr<ModelPartDefinition> wing0 = std::make_unique<ModelPartDefinition>("wing0");
    wing0->SetPosition(glm::dvec3(-4.0, 11.0, 0.0));
    wing0->AddCube(Box("wing0_cube", 24, 13, 0.0f, 0.0f, -3.0f, 1, 4, 6));

    std::unique_ptr<ModelPartDefinition> wing1 = std::make_unique<ModelPartDefinition>("wing1");
    wing1->SetPosition(glm::dvec3(4.0, 11.0, 0.0));
    wing1->AddCube(Box("wing1_cube", 24, 13, -1.0f, 0.0f, -3.0f, 1, 4, 6));

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    root->AddChild(std::move(leg0));
    root->AddChild(std::move(leg1));
    root->AddChild(std::move(wing0));
    root->AddChild(std::move(wing1));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSquid(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 16.0, 0.0));
    body->AddCube(Box("body_cube", 0, 0, -6.0f, -8.0f, -6.0f, 12, 16, 12));
    root->AddChild(std::move(body));
    for (int i = 0; i < 8; i++)
    {
        double angle                                  = i * std::numbers::pi * 2.0 / 8.0;
        std::string name                              = "tentacle" + std::to_string(i);
        std::unique_ptr<ModelPartDefinition> tentacle = std::make_unique<ModelPartDefinition>(name);
        tentacle->SetPosition(glm::dvec3(cos(angle) * 5.0, 9.0, -sin(angle) * 5.0));
        tentacle->SetRotation(glm::dvec3(0.0, i * 45.0 - 90.0, 0.0));
        tentacle->AddCube(Box(name + "_cube", 48, 0, -1.0f, 0.0f, -1.0f, 2, 18, 2));
        root->AddChild(std::move(tentacle));
    }

    return definition;
}

ModelDefinition ModelPartsAnimal::CreateOcelot(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 9.0, 9.0));
    head->AddCube(Box("head_cube", 0, 0, -2.5f, -2.0f, -3.0f, 5, 4, 5));
    head->AddCube(Box("nose_cube", 0, 24, -1.5f, 0.0f, -4.0f, 3, 2, 2));
    head->AddCube(Box("ear0_cube", 0, 10, -2.0f, -3.0f, 0.0f, 1, 1, 2));
    head->AddCube(Box("ear1_cube", 6, 10, 1.0f, -3.0f, 0.0f, 1, 1, 2));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 12.0, 10.0));
    body->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    body->AddCube(Box("body_cube", 20, 0, -2.0f, 3.0f, -8.0f, 4, 16, 6));

    std::unique_ptr<ModelPartDefinition> tail0 = std::make_unique<ModelPartDefinition>("tail0");
    tail0->SetPosition(glm::dvec3(0.0, 9.0, -8.0));
    tail0->SetRotation(glm::dvec3(51.57, 0.0, 0.0));
    tail0->AddCube(Box("tail0_cube", 0, 15, -0.5f, 0.0f, 0.0f, 1, 8, 1));

    std::unique_ptr<ModelPartDefinition> tail1 = std::make_unique<ModelPartDefinition>("tail1");
    tail1->SetPosition(glm::dvec3(0.0, 4.0, -14.0));
    tail1->SetRotation(glm::dvec3(99.0, 0.0, 0.0));
    tail1->AddCube(Box("tail1_cube", 4, 15, -0.5f, 0.0f, 0.0f, 1, 8, 1));

    const char *kLegNames[4] = {"leg0", "leg1", "leg2", "leg3"};
    const double kLegX[4]    = {-1.1, 1.1, -1.2, 1.2};
    const double kLegY[4]    = {6.0, 6.0, 10.2, 10.2};
    const double kLegZ[4]    = {-5.0, -5.0, 5.0, 5.0};
    for (int i = 0; i < 4; i++)
    {
        bool front                               = i >= 2;
        std::unique_ptr<ModelPartDefinition> leg = std::make_unique<ModelPartDefinition>(kLegNames[i]);
        leg->SetPosition(glm::dvec3(kLegX[i], kLegY[i], kLegZ[i]));
        leg->AddCube(
                Box(std::string(kLegNames[i]) + "_cube", front ? 40 : 8, front ? 0 : 13, -1.0f, 0.0f, front ? 0.0f : 1.0f, 2, front ? 10 : 6, 2));
        root->AddChild(std::move(leg));
    }

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    root->AddChild(std::move(tail0));
    root->AddChild(std::move(tail1));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateParrot(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 32, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 7.5, 3.0));
    body->SetRotation(glm::dvec3(28.29, 0.0, 0.0));
    body->AddCube(Box("body_cube", 2, 8, -1.5f, 0.0f, -1.5f, 3, 6, 3));

    std::unique_ptr<ModelPartDefinition> tail = std::make_unique<ModelPartDefinition>("tail");
    tail->SetPosition(glm::dvec3(0.0, 2.93, -1.16));
    tail->SetRotation(glm::dvec3(58.15, 0.0, 0.0));
    tail->AddCube(Box("tail_cube", 22, 1, -1.5f, -1.0f, -1.0f, 3, 4, 1));

    std::unique_ptr<ModelPartDefinition> wing0 = std::make_unique<ModelPartDefinition>("wing0");
    wing0->SetPosition(glm::dvec3(1.5, 7.06, 2.76));
    wing0->SetRotation(glm::dvec3(-40.0, 180.0, 0.0));
    wing0->AddCube(Box("wing0_cube", 19, 8, -0.5f, 0.0f, -1.5f, 1, 5, 3));

    std::unique_ptr<ModelPartDefinition> wing1 = std::make_unique<ModelPartDefinition>("wing1");
    wing1->SetPosition(glm::dvec3(-1.5, 7.06, 2.76));
    wing1->SetRotation(glm::dvec3(-40.0, 180.0, 0.0));
    wing1->AddCube(Box("wing1_cube", 19, 8, -0.5f, 0.0f, -1.5f, 1, 5, 3));

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 8.31, 2.76));
    head->AddCube(Box("head_cube", 2, 2, -1.0f, -1.5f, -1.0f, 2, 3, 2));

    std::unique_ptr<ModelPartDefinition> crest = std::make_unique<ModelPartDefinition>("crest");
    crest->SetPosition(glm::dvec3(0.0, 2.0, 1.0));
    crest->SetRotation(glm::dvec3(-12.69, 0.0, 0.0));
    crest->AddCube(Box("crest_cube", 10, 0, -1.0f, -0.5f, -2.0f, 2, 1, 4, -0.01f));

    std::unique_ptr<ModelPartDefinition> beak0 = std::make_unique<ModelPartDefinition>("beak0");
    beak0->SetPosition(glm::dvec3(0.0, 0.5, 1.5));
    beak0->SetRotation(glm::dvec3(-40.0, 0.0, 0.0));
    beak0->AddCube(Box("beak0_cube", 11, 7, -0.5f, -1.0f, -0.5f, 1, 2, 1));

    std::unique_ptr<ModelPartDefinition> beak1 = std::make_unique<ModelPartDefinition>("beak1");
    beak1->SetPosition(glm::dvec3(0.0, 1.75, 2.45));
    beak1->SetRotation(glm::dvec3(-2.5, 0.0, 0.0));
    beak1->AddCube(Box("beak1_cube", 16, 7, -0.5f, 0.0f, -0.5f, 1, 2, 1, -0.01f));

    std::unique_ptr<ModelPartDefinition> feather = std::make_unique<ModelPartDefinition>("feather");
    feather->SetPosition(glm::dvec3(0.0, 2.15, -0.15));
    feather->SetRotation(glm::dvec3(-12.69, 0.0, 0.0));
    feather->AddCube(Box("feather_cube", 2, 18, 0.0f, -4.0f, -2.0f, 0, 5, 4));

    head->AddChild(std::move(crest));
    head->AddChild(std::move(beak0));
    head->AddChild(std::move(beak1));
    head->AddChild(std::move(feather));

    std::unique_ptr<ModelPartDefinition> leg0 = std::make_unique<ModelPartDefinition>("leg0");
    leg0->SetPosition(glm::dvec3(1.0, 2.0, 1.05));
    leg0->AddCube(Box("leg0_cube", 14, 18, -0.5f, 0.0f, -0.5f, 1, 2, 1));

    std::unique_ptr<ModelPartDefinition> leg1 = std::make_unique<ModelPartDefinition>("leg1");
    leg1->SetPosition(glm::dvec3(-1.0, 2.0, 1.05));
    leg1->AddCube(Box("leg1_cube", 14, 18, -0.5f, 0.0f, -0.5f, 1, 2, 1));

    root->AddChild(std::move(body));
    root->AddChild(std::move(tail));
    root->AddChild(std::move(wing0));
    root->AddChild(std::move(wing1));
    root->AddChild(std::move(head));
    root->AddChild(std::move(leg0));
    root->AddChild(std::move(leg1));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSpider(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 9.0, 3.0));
    head->AddCube(Box("head_cube", 32, 4, -4.0f, -4.0f, -8.0f, 8, 8, 8));

    std::unique_ptr<ModelPartDefinition> neck = std::make_unique<ModelPartDefinition>("neck");
    neck->SetPosition(glm::dvec3(0.0, 9.0, 0.0));
    neck->AddCube(Box("neck_cube", 0, 0, -3.0f, -3.0f, -3.0f, 6, 6, 6));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 9.0, -9.0));
    body->AddCube(Box("body_cube", 0, 12, -5.0f, -4.0f, -6.0f, 10, 8, 12));

    root->AddChild(std::move(head));
    root->AddChild(std::move(neck));
    root->AddChild(std::move(body));
    for (int i = 0; i < 8; i++)
    {
        std::string name                         = "leg" + std::to_string(i);
        std::unique_ptr<ModelPartDefinition> leg = std::make_unique<ModelPartDefinition>(name);
        leg->SetPosition(glm::dvec3(i % 2 == 0 ? -4.0 : 4.0, 9.0, i / 2 - 2.0));
        leg->AddCube(Box(name + "_cube", 18, 0, i % 2 == 0 ? -15.0f : -1.0f, -1.0f, -1.0f, 16, 2, 2));
        root->AddChild(std::move(leg));
    }

    return definition;
}

ModelDefinition ModelPartsAnimal::CreateCreeper(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 18.0, 0.0));
    head->AddCube(Box("head_cube", 0, 0, -4.0f, -8.0f, -4.0f, 8, 8, 8));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 18.0, 0.0));
    body->AddCube(Box("body_cube", 16, 16, -4.0f, 0.0f, -2.0f, 8, 12, 4));

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    const double kLegX[4] = {-2.0, 2.0, -2.0, 2.0};
    const double kLegZ[4] = {-4.0, -4.0, 4.0, 4.0};
    for (int i = 0; i < 4; i++)
    {
        std::string name                         = "leg" + std::to_string(i);
        std::unique_ptr<ModelPartDefinition> leg = std::make_unique<ModelPartDefinition>(name);
        leg->SetPosition(glm::dvec3(kLegX[i], 6.0, kLegZ[i]));
        leg->AddCube(Box(name + "_cube", 0, 16, -2.0f, 0.0f, -2.0f, 4, 6, 4));
        root->AddChild(std::move(leg));
    }

    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSlime(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 24.0, 0.0));
    body->AddCube(Box("body_cube", 0, 16, -3.0f, 17.0f, -3.0f, 6, 6, 6));
    body->AddCube(Box("eye0", 32, 0, -3.25f, 18.0f, -3.5f, 2, 2, 2));
    body->AddCube(Box("eye1", 32, 4, 1.25f, 18.0f, -3.5f, 2, 2, 2));
    body->AddCube(Box("mouth", 32, 8, 0.0f, 21.0f, -3.5f, 1, 1, 1));
    definition.GetRoot()->AddChild(std::move(body));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateSlimeOuter(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 24.0, 0.0));
    body->AddCube(Box("body_cube", 0, 0, -4.0f, 16.0f, -4.0f, 8, 8, 8));
    definition.GetRoot()->AddChild(std::move(body));
    return definition;
}

// We flip these boxes because they are laid out y down and -z forward while our models are y up and +z forward
ModelPartCubeDefinition ModelPartsAnimal::Box(const std::string &name, int u, int v, float x, float y, float z, int width, int height, int depth,
                                              float grow)
{
    ModelPartCubeDefinition::FaceUV uv;
    uv.left   = {u, v + depth, depth, height};
    uv.right  = {u + depth + width, v + depth, depth, height};
    uv.front  = {u + depth, v + depth, width, height};
    uv.back   = {u + depth * 2 + width, v + depth, width, height};
    uv.top    = {u + depth, v, width, depth};
    uv.bottom = {u + depth + width, v + depth, width, -depth};
    return ModelPartCubeDefinition(name, glm::dvec3(x, -(y + height), -(z + depth)), glm::dvec3(x + width, -y, -z), uv, grow);
}

ModelDefinition ModelPartsAnimal::CreateBoat(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 128, 64));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> side0 = std::make_unique<ModelPartDefinition>("side0");
    side0->SetPosition(glm::dvec3(0.0, -3.0, -1.0));
    side0->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    side0->AddCube(Box("side0_cube", 0, 0, -14.0f, -9.0f, -3.0f, 28, 16, 3));

    std::unique_ptr<ModelPartDefinition> side1 = std::make_unique<ModelPartDefinition>("side1");
    side1->SetPosition(glm::dvec3(-15.0, -4.0, -4.0));
    side1->SetRotation(glm::dvec3(0.0, -270.0, 0.0));
    side1->AddCube(Box("side1_cube", 0, 19, -13.0f, -7.0f, -1.0f, 18, 6, 2));

    std::unique_ptr<ModelPartDefinition> side2 = std::make_unique<ModelPartDefinition>("side2");
    side2->SetPosition(glm::dvec3(15.0, -4.0, 0.0));
    side2->SetRotation(glm::dvec3(0.0, -90.0, 0.0));
    side2->AddCube(Box("side2_cube", 0, 27, -8.0f, -7.0f, -1.0f, 16, 6, 2));

    std::unique_ptr<ModelPartDefinition> side3 = std::make_unique<ModelPartDefinition>("side3");
    side3->SetPosition(glm::dvec3(0.0, -4.0, 9.0));
    side3->SetRotation(glm::dvec3(0.0, -180.0, 0.0));
    side3->AddCube(Box("side3_cube", 0, 35, -14.0f, -7.0f, -1.0f, 28, 6, 2));

    std::unique_ptr<ModelPartDefinition> side4 = std::make_unique<ModelPartDefinition>("side4");
    side4->SetPosition(glm::dvec3(0.0, -4.0, -9.0));
    side4->AddCube(Box("side4_cube", 0, 43, -14.0f, -7.0f, -1.0f, 28, 6, 2));

    std::unique_ptr<ModelPartDefinition> paddle0 = std::make_unique<ModelPartDefinition>("paddle0");
    paddle0->SetPosition(glm::dvec3(3.0, 5.0, -9.0));
    paddle0->SetRotation(glm::dvec3(0.0, 0.0, -11.25));
    paddle0->AddCube(Box("paddle0_shaft", 62, 0, -1.0f, 0.0f, -5.0f, 2, 2, 18));
    paddle0->AddCube(Box("paddle0_blade", 62, 0, -1.001f, -3.0f, 8.0f, 1, 6, 7));

    std::unique_ptr<ModelPartDefinition> paddle1 = std::make_unique<ModelPartDefinition>("paddle1");
    paddle1->SetPosition(glm::dvec3(3.0, 5.0, 9.0));
    paddle1->SetRotation(glm::dvec3(0.0, -180.0, -11.25));
    paddle1->AddCube(Box("paddle1_shaft", 62, 20, -1.0f, 0.0f, -5.0f, 2, 2, 18));
    paddle1->AddCube(Box("paddle1_blade", 62, 20, 0.001f, -3.0f, 8.0f, 1, 6, 7));

    root->AddChild(std::move(side0));
    root->AddChild(std::move(side1));
    root->AddChild(std::move(side2));
    root->AddChild(std::move(side3));
    root->AddChild(std::move(side4));
    root->AddChild(std::move(paddle0));
    root->AddChild(std::move(paddle1));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateBoatWaterPatch(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 128, 64));
    std::unique_ptr<ModelPartDefinition> patch = std::make_unique<ModelPartDefinition>("patch");
    patch->SetPosition(glm::dvec3(0.0, 3.0, -1.0));
    patch->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    patch->AddCube(Box("patch_cube", 0, 0, -14.0f, -9.0f, -3.0f, 28, 16, 3));
    definition.GetRoot()->AddChild(std::move(patch));
    return definition;
}

ModelDefinition ModelPartsAnimal::CreateMinecart(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> bottom = std::make_unique<ModelPartDefinition>("bottom");
    bottom->SetPosition(glm::dvec3(0.0, -4.0, 0.0));
    bottom->SetRotation(glm::dvec3(90.0, 0.0, 0.0));
    bottom->AddCube(Box("bottom_cube", 0, 10, -10.0f, -8.0f, -1.0f, 20, 16, 2));
    root->AddChild(std::move(bottom));

    std::unique_ptr<ModelPartDefinition> contents = std::make_unique<ModelPartDefinition>("contents");
    contents->SetPosition(glm::dvec3(0.0, -4.1, 0.0));
    contents->SetRotation(glm::dvec3(-90.0, 0.0, 0.0));
    contents->AddCube(Box("contents_cube", 44, 10, -9.0f, -7.0f, -1.0f, 18, 14, 1));
    root->AddChild(std::move(contents));

    const glm::dvec3 sidePositions[4] = {glm::dvec3(-9.0, -4.0, 0.0), glm::dvec3(9.0, -4.0, 0.0), glm::dvec3(0.0, -4.0, 7.0),
                                         glm::dvec3(0.0, -4.0, -7.0)};
    const double sideYaws[4]          = {-270.0, -90.0, -180.0, 0.0};
    for (int i = 0; i < 4; i++)
    {
        std::string name                          = "side" + std::to_string(i);
        std::unique_ptr<ModelPartDefinition> side = std::make_unique<ModelPartDefinition>(name);
        side->SetPosition(sidePositions[i]);
        side->SetRotation(glm::dvec3(0.0, sideYaws[i], 0.0));
        side->AddCube(Box(name + "_cube", 0, 0, -8.0f, -9.0f, -1.0f, 16, 8, 2));
        root->AddChild(std::move(side));
    }

    return definition;
}

ModelDefinition ModelPartsAnimal::CreateLeashKnot(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 32, 32));
    std::unique_ptr<ModelPartDefinition> knot = std::make_unique<ModelPartDefinition>("knot");
    knot->AddCube(Box("knot_cube", 0, 0, -3.0f, -6.0f, -3.0f, 6, 8, 6));
    definition.GetRoot()->AddChild(std::move(knot));
    return definition;
}

#endif
