#include "Recipe.h"

Recipe::Recipe(const ItemInstance &result)
{
    m_result = result;
}

const ItemInstance &Recipe::GetResult() const
{
    return m_result;
}
