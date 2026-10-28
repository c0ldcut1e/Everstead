#include "EggItem.h"

#include "../../entity/projectile/ThrownEgg.h"

ThrowableProjectile *EggItem::CreateProjectile(Level *level) const
{
    return new ThrownEgg(level);
}
