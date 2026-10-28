#include "RecallFracture.h"

#include "../../entity/projectile/Arrow.h"
#include "../Level.h"
#include "FractureTraits.h"

RecallFracture::RecallFracture() : FractureTrait("recall", FractureTraits::eGearType_Bow, 3)
{}

bool RecallFracture::OnArrowLand(Arrow *arrow, Direction *face)
{
    (void) face;
    if (arrow->GetLevel()->GetRandom().NextFloat() >= 0.75f)
    {
        return false;
    }

    arrow->ReturnToOwner();
    SendParticles(arrow->GetLevel(), eParticleType_Rift, arrow->GetPosition(), glm::dvec3(0.1), 0.05f, 6);
    PlaySound(arrow->GetLevel(), arrow->GetPosition(), 0.7f);
    return true;
}
