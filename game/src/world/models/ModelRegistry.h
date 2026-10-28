#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../../rendering/TextureRepository.h"
#include "Model.h"

class ModelRegistry
{
public:
    static ModelRegistry *GetInstance();
    static void Init();

    void RegisterModel(const std::string &name, std::unique_ptr<Model> model);
    Model *GetModel(const std::string &name) const;
    bool HasModel(const std::string &name) const;

    TextureRepository *GetTextures();

private:
    ModelRegistry() = default;

    std::unordered_map<std::string, std::unique_ptr<Model>> m_models;
};
