#include "AbstractContainer.h"

#include <algorithm>

int AbstractContainer::GetRedstoneSignal() const
{
    int nonEmpty   = 0;
    float fullness = 0.0f;
    for (int i = 0; i < GetSize(); i++)
    {
        const ItemInstance *item = GetItem(i);
        if (!item->IsEmpty())
        {
            fullness += (float) item->GetCount() / (float) std::min(64, item->GetMaxStackSize());
            nonEmpty++;
        }
    }

    return (int) (fullness / (float) GetSize() * 14.0f) + (nonEmpty > 0 ? 1 : 0);
}
