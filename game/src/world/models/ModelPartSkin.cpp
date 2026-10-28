#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPartSkin.h"

ModelPartSkin::ModelPartSkin()
{
    m_texturePath   = "";
    m_textureWidth  = 0;
    m_textureHeight = 0;
    m_type          = eModelPartSkinType_Normal;
}

ModelPartSkin::ModelPartSkin(const std::string &texturePath, int textureWidth, int textureHeight)
{
    m_texturePath   = texturePath;
    m_textureWidth  = textureWidth;
    m_textureHeight = textureHeight;
    m_type          = eModelPartSkinType_Normal;
}

ModelPartSkin::ModelPartSkin(const std::string &texturePath, int textureWidth, int textureHeight, eMODEL_PART_SKIN_TYPE type)
{
    m_texturePath   = texturePath;
    m_textureWidth  = textureWidth;
    m_textureHeight = textureHeight;
    m_type          = type;
}

const std::string &ModelPartSkin::GetTexturePath() const
{
    return m_texturePath;
}

int ModelPartSkin::GetTextureWidth() const
{
    return m_textureWidth;
}

int ModelPartSkin::GetTextureHeight() const
{
    return m_textureHeight;
}

eMODEL_PART_SKIN_TYPE ModelPartSkin::GetType() const
{
    return m_type;
}

#endif
