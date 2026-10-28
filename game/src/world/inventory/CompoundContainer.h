#pragma once

#include "AbstractContainer.h"

class CompoundContainer : public AbstractContainer
{
public:
    CompoundContainer(AbstractContainer *first, AbstractContainer *second);
    ~CompoundContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

    AbstractContainer *GetFirst() const;
    AbstractContainer *GetSecond() const;

private:
    AbstractContainer *m_first;
    AbstractContainer *m_second;
};
