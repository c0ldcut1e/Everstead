#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelDefinition.h"

ModelDefinition::ModelDefinition()
{
    m_id   = "";
    m_skin = {};
    m_root = std::make_unique<ModelPartDefinition>("root");
}

ModelDefinition::ModelDefinition(const std::string &id, const ModelPartSkin &skin)
{
    m_id   = id;
    m_skin = skin;
    m_root = std::make_unique<ModelPartDefinition>("root");
}

const std::string &ModelDefinition::GetId() const
{
    return m_id;
}

void ModelDefinition::SetSkin(const ModelPartSkin &skin)
{
    m_skin = skin;
}

const ModelPartSkin &ModelDefinition::GetSkin() const
{
    return m_skin;
}

ModelPartDefinition *ModelDefinition::GetRoot()
{
    return m_root.get();
}

const ModelPartDefinition *ModelDefinition::GetRoot() const
{
    return m_root.get();
}

#endif
