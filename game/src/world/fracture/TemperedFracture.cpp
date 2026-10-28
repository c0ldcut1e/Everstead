#include "TemperedFracture.h"

#include "FractureTraits.h"

TemperedFracture::TemperedFracture() : FractureTrait("tempered", FractureTraits::eGearType_All, 3)
{}

int TemperedFracture::ModifyWear(int amount, Random *random)
{
    int wear = 0;
    for (int i = 0; i < amount; i++)
    {
        if (random->NextFloat() >= 0.4f)
        {
            wear++;
        }
    }

    return wear;
}
