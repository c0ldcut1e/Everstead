#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPartsSkinned.h"

ModelPartCubeDefinition::FaceUV UVCube(int leftX, int leftY, int rightX, int rightY, int frontX, int frontY, int backX, int backY, int topX, int topY,
                                       int bottomX, int bottomY, int sideWidth, int sideHeight, int frontWidth, int frontHeight, int topWidth,
                                       int topHeight)
{
    ModelPartCubeDefinition::FaceUV uv;
    uv.left   = {leftX, leftY, sideWidth, sideHeight};
    uv.right  = {rightX, rightY, sideWidth, sideHeight};
    uv.front  = {frontX, frontY, frontWidth, frontHeight};
    uv.back   = {backX, backY, frontWidth, frontHeight};
    uv.top    = {topX, topY, topWidth, topHeight};
    uv.bottom = {bottomX, bottomY, topWidth, topHeight};
    return uv;
}

ModelDefinition ModelPartsSkinned::CreateSteveClassic(const std::string &id, const std::string &texturePath)
{
    return CreatePlayer(id, texturePath, ePlayerSkinModel_Classic);
}

ModelDefinition ModelPartsSkinned::CreateSteveSlim(const std::string &id, const std::string &texturePath)
{
    return CreatePlayer(id, texturePath, ePlayerSkinModel_Slim);
}

ModelDefinition ModelPartsSkinned::CreatePlayer(const std::string &id, const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 64, eModelPartSkinType_Player));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    head->AddCube(ModelPartCubeDefinition("head_cube", glm::dvec3(-4.0, 0.0, -4.0), glm::dvec3(4.0, 8.0, 4.0),
                                          UVCube(0, 8, 16, 8, 8, 8, 24, 8, 8, 0, 16, 0, 8, 8, 8, 8, 8, 8), 0.0f));
    head->AddCube(ModelPartCubeDefinition("hat_cube", glm::dvec3(-4.0, 0.0, -4.0), glm::dvec3(4.0, 8.0, 4.0),
                                          UVCube(32, 8, 48, 8, 40, 8, 56, 8, 40, 0, 48, 0, 8, 8, 8, 8, 8, 8), 0.5f));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    body->AddCube(ModelPartCubeDefinition("body_cube", glm::dvec3(-4.0, -12.0, -2.0), glm::dvec3(4.0, 0.0, 2.0),
                                          UVCube(16, 20, 28, 20, 20, 20, 32, 20, 20, 16, 28, 16, 4, 12, 8, 12, 8, 4), 0.0f));
    body->AddCube(ModelPartCubeDefinition("jacket_cube", glm::dvec3(-4.0, -12.0, -2.0), glm::dvec3(4.0, 0.0, 2.0),
                                          UVCube(16, 36, 28, 36, 20, 36, 32, 36, 20, 32, 28, 32, 4, 12, 8, 12, 8, 4), 0.25f));

    int armWidth          = 4;
    double rightArmMin    = -3.0;
    double rightArmMax    = 1.0;
    double leftArmMin     = -1.0;
    double leftArmMax     = 3.0;
    int rightArmRightX    = 48;
    int rightArmBackX     = 52;
    int rightArmDownX     = 48;
    int leftArmRightX     = 40;
    int leftArmBackX      = 44;
    int leftArmDownX      = 40;
    int rightSleeveRightX = 48;
    int rightSleeveBackX  = 52;
    int rightSleeveDownX  = 48;
    int leftSleeveRightX  = 56;
    int leftSleeveBackX   = 60;
    int leftSleeveDownX   = 56;
    if (skinModel == ePlayerSkinModel_Slim)
    {
        armWidth          = 3;
        rightArmMin       = -2.0;
        rightArmMax       = 1.0;
        leftArmMin        = -1.0;
        leftArmMax        = 2.0;
        rightArmRightX    = 47;
        rightArmBackX     = 51;
        rightArmDownX     = 47;
        leftArmRightX     = 39;
        leftArmBackX      = 43;
        leftArmDownX      = 39;
        rightSleeveRightX = 47;
        rightSleeveBackX  = 51;
        rightSleeveDownX  = 47;
        leftSleeveRightX  = 55;
        leftSleeveBackX   = 59;
        leftSleeveDownX   = 55;
    }

    std::unique_ptr<ModelPartDefinition> rightArm = std::make_unique<ModelPartDefinition>("rightArm");
    rightArm->SetPosition(glm::dvec3(-5.0, 20.0, 0.0));
    rightArm->AddCube(ModelPartCubeDefinition(
            "right_arm_cube", glm::dvec3(rightArmMin, -10.0, -2.0), glm::dvec3(rightArmMax, 2.0, 2.0),
            UVCube(40, 20, rightArmRightX, 20, 44, 20, rightArmBackX, 20, 44, 16, rightArmDownX, 16, 4, 12, armWidth, 12, armWidth, 4), 0.0f));
    rightArm->AddCube(ModelPartCubeDefinition(
            "right_sleeve_cube", glm::dvec3(rightArmMin, -10.0, -2.0), glm::dvec3(rightArmMax, 2.0, 2.0),
            UVCube(40, 36, rightSleeveRightX, 36, 44, 36, rightSleeveBackX, 36, 44, 32, rightSleeveDownX, 32, 4, 12, armWidth, 12, armWidth, 4),
            0.25f));

    std::unique_ptr<ModelPartDefinition> leftArm = std::make_unique<ModelPartDefinition>("leftArm");
    leftArm->SetPosition(glm::dvec3(5.0, 20.0, 0.0));
    leftArm->AddCube(ModelPartCubeDefinition(
            "left_arm_cube", glm::dvec3(leftArmMin, -10.0, -2.0), glm::dvec3(leftArmMax, 2.0, 2.0),
            UVCube(32, 52, leftArmRightX, 52, 36, 52, leftArmBackX, 52, 36, 48, leftArmDownX, 48, 4, 12, armWidth, 12, armWidth, 4), 0.0f));
    leftArm->AddCube(ModelPartCubeDefinition(
            "left_sleeve_cube", glm::dvec3(leftArmMin, -10.0, -2.0), glm::dvec3(leftArmMax, 2.0, 2.0),
            UVCube(48, 52, leftSleeveRightX, 52, 52, 52, leftSleeveBackX, 52, 52, 48, leftSleeveDownX, 48, 4, 12, armWidth, 12, armWidth, 4), 0.25f));

    std::unique_ptr<ModelPartDefinition> rightLeg = std::make_unique<ModelPartDefinition>("rightLeg");
    rightLeg->SetPosition(glm::dvec3(-1.9, 10.0, 0.0));
    rightLeg->AddCube(ModelPartCubeDefinition("right_leg_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0),
                                              UVCube(0, 20, 8, 20, 4, 20, 12, 20, 4, 16, 8, 16, 4, 12, 4, 12, 4, 4), 0.0f));
    rightLeg->AddCube(ModelPartCubeDefinition("right_pants_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0),
                                              UVCube(0, 36, 8, 36, 4, 36, 12, 36, 4, 32, 8, 32, 4, 12, 4, 12, 4, 4), 0.25f));

    std::unique_ptr<ModelPartDefinition> leftLeg = std::make_unique<ModelPartDefinition>("leftLeg");
    leftLeg->SetPosition(glm::dvec3(1.9, 10.0, 0.0));
    leftLeg->AddCube(ModelPartCubeDefinition("left_leg_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0),
                                             UVCube(16, 52, 24, 52, 20, 52, 28, 52, 20, 48, 24, 48, 4, 12, 4, 12, 4, 4), 0.0f));
    leftLeg->AddCube(ModelPartCubeDefinition("left_pants_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0),
                                             UVCube(0, 52, 8, 52, 4, 52, 12, 52, 4, 48, 8, 48, 4, 12, 4, 12, 4, 4), 0.25f));

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    root->AddChild(std::move(rightArm));
    root->AddChild(std::move(leftArm));
    root->AddChild(std::move(rightLeg));
    root->AddChild(std::move(leftLeg));

    return definition;
}

ModelDefinition ModelPartsSkinned::CreateHumanoid(const std::string &id, const std::string &texturePath, int textureHeight, float inflate)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, textureHeight));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    head->AddCube(ModelPartCubeDefinition("head_cube", glm::dvec3(-4.0, 0.0, -4.0), glm::dvec3(4.0, 8.0, 4.0),
                                          UVCube(0, 8, 16, 8, 8, 8, 24, 8, 8, 0, 16, 0, 8, 8, 8, 8, 8, 8), inflate));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    body->AddCube(ModelPartCubeDefinition("body_cube", glm::dvec3(-4.0, -12.0, -2.0), glm::dvec3(4.0, 0.0, 2.0),
                                          UVCube(16, 20, 28, 20, 20, 20, 32, 20, 20, 16, 28, 16, 4, 12, 8, 12, 8, 4), inflate));

    // Negative widths flip the texels so the left limbs mirror the right ones
    ModelPartCubeDefinition::FaceUV mirroredArm;
    mirroredArm.left   = {52, 20, -4, 12};
    mirroredArm.right  = {44, 20, -4, 12};
    mirroredArm.front  = {48, 20, -4, 12};
    mirroredArm.back   = {56, 20, -4, 12};
    mirroredArm.top    = {48, 16, -4, 4};
    mirroredArm.bottom = {52, 16, -4, 4};
    ModelPartCubeDefinition::FaceUV mirroredLeg;
    mirroredLeg.left   = {12, 20, -4, 12};
    mirroredLeg.right  = {4, 20, -4, 12};
    mirroredLeg.front  = {8, 20, -4, 12};
    mirroredLeg.back   = {16, 20, -4, 12};
    mirroredLeg.top    = {8, 16, -4, 4};
    mirroredLeg.bottom = {12, 16, -4, 4};

    std::unique_ptr<ModelPartDefinition> rightArm = std::make_unique<ModelPartDefinition>("rightArm");
    rightArm->SetPosition(glm::dvec3(-5.0, 20.0, 0.0));
    rightArm->AddCube(ModelPartCubeDefinition("right_arm_cube", glm::dvec3(-3.0, -10.0, -2.0), glm::dvec3(1.0, 2.0, 2.0),
                                              UVCube(40, 20, 48, 20, 44, 20, 52, 20, 44, 16, 48, 16, 4, 12, 4, 12, 4, 4), inflate));

    std::unique_ptr<ModelPartDefinition> leftArm = std::make_unique<ModelPartDefinition>("leftArm");
    leftArm->SetPosition(glm::dvec3(5.0, 20.0, 0.0));
    leftArm->AddCube(ModelPartCubeDefinition("left_arm_cube", glm::dvec3(-1.0, -10.0, -2.0), glm::dvec3(3.0, 2.0, 2.0), mirroredArm, inflate));

    std::unique_ptr<ModelPartDefinition> rightLeg = std::make_unique<ModelPartDefinition>("rightLeg");
    rightLeg->SetPosition(glm::dvec3(-1.9, 10.0, 0.0));
    rightLeg->AddCube(ModelPartCubeDefinition("right_leg_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0),
                                              UVCube(0, 20, 8, 20, 4, 20, 12, 20, 4, 16, 8, 16, 4, 12, 4, 12, 4, 4), inflate));

    std::unique_ptr<ModelPartDefinition> leftLeg = std::make_unique<ModelPartDefinition>("leftLeg");
    leftLeg->SetPosition(glm::dvec3(1.9, 10.0, 0.0));
    leftLeg->AddCube(ModelPartCubeDefinition("left_leg_cube", glm::dvec3(-2.0, -12.0, -2.0), glm::dvec3(2.0, 0.0, 2.0), mirroredLeg, inflate));

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    root->AddChild(std::move(rightArm));
    root->AddChild(std::move(leftArm));
    root->AddChild(std::move(rightLeg));
    root->AddChild(std::move(leftLeg));

    return definition;
}

ModelDefinition ModelPartsSkinned::CreateSkeleton(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> head = std::make_unique<ModelPartDefinition>("head");
    head->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    head->AddCube(ModelPartCubeDefinition("head_cube", glm::dvec3(-4.0, 0.0, -4.0), glm::dvec3(4.0, 8.0, 4.0),
                                          UVCube(0, 8, 16, 8, 8, 8, 24, 8, 8, 0, 16, 0, 8, 8, 8, 8, 8, 8), 0.0f));

    std::unique_ptr<ModelPartDefinition> body = std::make_unique<ModelPartDefinition>("body");
    body->SetPosition(glm::dvec3(0.0, 22.0, 0.0));
    body->AddCube(ModelPartCubeDefinition("body_cube", glm::dvec3(-4.0, -12.0, -2.0), glm::dvec3(4.0, 0.0, 2.0),
                                          UVCube(16, 20, 28, 20, 20, 20, 32, 20, 20, 16, 28, 16, 4, 12, 8, 12, 8, 4), 0.0f));

    ModelPartCubeDefinition::FaceUV mirroredArm;
    mirroredArm.left   = {46, 18, -2, 12};
    mirroredArm.right  = {42, 18, -2, 12};
    mirroredArm.front  = {44, 18, -2, 12};
    mirroredArm.back   = {48, 18, -2, 12};
    mirroredArm.top    = {44, 16, -2, 2};
    mirroredArm.bottom = {46, 16, -2, 2};
    ModelPartCubeDefinition::FaceUV mirroredLeg;
    mirroredLeg.left   = {6, 18, -2, 12};
    mirroredLeg.right  = {2, 18, -2, 12};
    mirroredLeg.front  = {4, 18, -2, 12};
    mirroredLeg.back   = {8, 18, -2, 12};
    mirroredLeg.top    = {4, 16, -2, 2};
    mirroredLeg.bottom = {6, 16, -2, 2};

    std::unique_ptr<ModelPartDefinition> rightArm = std::make_unique<ModelPartDefinition>("rightArm");
    rightArm->SetPosition(glm::dvec3(-5.0, 20.0, 0.0));
    rightArm->AddCube(ModelPartCubeDefinition("right_arm_cube", glm::dvec3(-1.0, -10.0, -1.0), glm::dvec3(1.0, 2.0, 1.0),
                                              UVCube(40, 18, 44, 18, 42, 18, 46, 18, 42, 16, 44, 16, 2, 12, 2, 12, 2, 2), 0.0f));

    std::unique_ptr<ModelPartDefinition> leftArm = std::make_unique<ModelPartDefinition>("leftArm");
    leftArm->SetPosition(glm::dvec3(5.0, 20.0, 0.0));
    leftArm->AddCube(ModelPartCubeDefinition("left_arm_cube", glm::dvec3(-1.0, -10.0, -1.0), glm::dvec3(1.0, 2.0, 1.0), mirroredArm, 0.0f));

    std::unique_ptr<ModelPartDefinition> rightLeg = std::make_unique<ModelPartDefinition>("rightLeg");
    rightLeg->SetPosition(glm::dvec3(-2.0, 10.0, 0.0));
    rightLeg->AddCube(ModelPartCubeDefinition("right_leg_cube", glm::dvec3(-1.0, -12.0, -1.0), glm::dvec3(1.0, 0.0, 1.0),
                                              UVCube(0, 18, 4, 18, 2, 18, 6, 18, 2, 16, 4, 16, 2, 12, 2, 12, 2, 2), 0.0f));

    std::unique_ptr<ModelPartDefinition> leftLeg = std::make_unique<ModelPartDefinition>("leftLeg");
    leftLeg->SetPosition(glm::dvec3(2.0, 10.0, 0.0));
    leftLeg->AddCube(ModelPartCubeDefinition("left_leg_cube", glm::dvec3(-1.0, -12.0, -1.0), glm::dvec3(1.0, 0.0, 1.0), mirroredLeg, 0.0f));

    root->AddChild(std::move(head));
    root->AddChild(std::move(body));
    root->AddChild(std::move(rightArm));
    root->AddChild(std::move(leftArm));
    root->AddChild(std::move(rightLeg));
    root->AddChild(std::move(leftLeg));

    return definition;
}

ModelDefinition ModelPartsSkinned::CreateCape(const std::string &id, const std::string &texturePath)
{
    ModelDefinition definition(id, ModelPartSkin(texturePath, 64, 32, eModelPartSkinType_Cape));
    ModelPartDefinition *root = definition.GetRoot();

    std::unique_ptr<ModelPartDefinition> cape = std::make_unique<ModelPartDefinition>("cape");
    cape->SetPosition(glm::dvec3(0.0, 22.0, -2.0));
    cape->AddCube(ModelPartCubeDefinition("cape_cube", glm::dvec3(-5.0, -16.0, -1.0), glm::dvec3(5.0, 0.0, 0.0),
                                          UVCube(0, 1, 11, 1, 12, 1, 1, 1, 1, 0, 11, 0, 1, 16, 10, 16, 10, 1), 0.0f));

    root->AddChild(std::move(cape));
    return definition;
}

#endif
