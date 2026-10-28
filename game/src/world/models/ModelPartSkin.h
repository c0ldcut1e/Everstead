#pragma once

#include <string>

enum eMODEL_PART_SKIN_TYPE
{
    eModelPartSkinType_Normal = 0,
    eModelPartSkinType_Player,
    eModelPartSkinType_Cape,
};

class ModelPartSkin
{
public:
    ModelPartSkin();
    ModelPartSkin(const std::string &texturePath, int textureWidth, int textureHeight);
    ModelPartSkin(const std::string &texturePath, int textureWidth, int textureHeight,
                  eMODEL_PART_SKIN_TYPE type);

    const std::string &GetTexturePath() const;
    int GetTextureWidth() const;
    int GetTextureHeight() const;
    eMODEL_PART_SKIN_TYPE GetType() const;

private:
    std::string m_texturePath;
    int m_textureWidth;
    int m_textureHeight;
    eMODEL_PART_SKIN_TYPE m_type;
};
