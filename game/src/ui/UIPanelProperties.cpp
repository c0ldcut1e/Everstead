#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIPanelProperties.h"

UIPanelProperties::UIPanelProperties(float _x, float _y, float _width, float _height, float _transparency, void *_customProperties,
                                     float _cornerScale)
{
    x                = _x;
    y                = _y;
    width            = _width;
    height           = _height;
    transparency     = _transparency;
    customProperties = _customProperties;
    cornerScale      = _cornerScale;
}

UIPanelProperties::~UIPanelProperties()
{}

float UIPanelProperties::GetSliceRatio() const
{
    return 12.0f / 96.0f;
}

#endif
