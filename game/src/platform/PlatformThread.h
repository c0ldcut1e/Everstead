#pragma once

class PlatformThread
{
public:
    static void LowerCurrentPriority(bool background);
};
