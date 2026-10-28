#pragma once

#include "../item/ItemInstance.h"

class AbstractContainer
{
public:
    virtual ~AbstractContainer() = default;

    virtual int GetColumns() const = 0;
    virtual int GetRows() const    = 0;
    virtual int GetSize() const    = 0;

    virtual ItemInstance *GetItem(int index)                  = 0;
    virtual const ItemInstance *GetItem(int index) const      = 0;
    virtual void SetItem(int index, const ItemInstance &item) = 0;
    virtual void ClearItem(int index)                         = 0;

    virtual bool IsEmpty() const = 0;

    int GetRedstoneSignal() const;
};
