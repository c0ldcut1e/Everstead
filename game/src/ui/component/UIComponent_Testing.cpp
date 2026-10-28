#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Testing.h"

#include "core/Everstead.h"
#include "entity/EntityFactory.h"
#include "entity/EntityRendererRegistry.h"
#include "entity/EntityType.h"

UIComponent_Testing::UIComponent_Testing() : UIComponent()
{}

void UIComponent_Testing::Render()
{
    // static std::unique_ptr<Entity> entity = EntityFactory::Create(eEntityType_Skeleton, Everstead::GetInstance()->GetLevel());
    // EntityRendererRegistry::GetInstance()->GetValue(eEntityType_Skeleton)->Render(entity.get(), 0.0f, 0x000000);
}

#endif
