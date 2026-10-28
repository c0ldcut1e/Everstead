#ifndef EVERSTEAD_DEDICATED_SERVER

#include "RenderDevice.h"

RenderDevice::RenderDevice()
{
    s_instance = this;
}

RenderDevice::~RenderDevice()
{
    s_instance = nullptr;
}

#endif
