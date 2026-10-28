#pragma once

#include "TextureAtlas.h"

class ItemAtlas
{
public:
    static ItemAtlas *GetInstance();

    void Build();
    Texture *GetTexture() const;
    TextureAtlas *GetAtlas();

private:
    TextureAtlas m_atlas;
};
