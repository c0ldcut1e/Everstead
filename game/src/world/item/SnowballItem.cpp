#include "SnowballItem.h"

#include "../../entity/projectile/Snowball.h"

ThrowableProjectile *SnowballItem::CreateProjectile(Level *level) const
{
    return new Snowball(level);
}
