#pragma once

class UIPanelProperties
{
public:
    UIPanelProperties(float _x, float _y, float _width, float _height, float _transparency = 1.0f, void *_customProperties = nullptr,
                      float _cornerScale = 1.0f);
    virtual ~UIPanelProperties();

    virtual const char *GetTexturePath() const = 0;
    virtual float GetSliceRatio() const;

    float x;
    float y;
    float width;
    float height;
    float transparency;
    void *customProperties;
    float cornerScale;
};
