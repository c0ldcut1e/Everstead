#pragma once

#include <string>
#include <vector>

#include "Command.h"
#include "CommandContext.h"

#include <nlohmann/json.hpp>

class CommandDispatcher;

class ExecuteCommand : public Command
{
public:
    explicit ExecuteCommand(CommandDispatcher *dispatcher);
    ~ExecuteCommand() override;

    bool RequiresOperator() const override;
    void Execute(CommandContext *context, const CommandArguments &arguments) override;
    void Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                  std::vector<std::string> *completions) const override;

private:
    bool ApplyModifier(const std::string &name, const CommandArguments &arguments, size_t *index, const CommandContext &context,
                       std::vector<CommandContext> *results, nlohmann::json *errorMessage) const;
    bool GetConditionLength(const CommandArguments &arguments, size_t index, size_t *length, nlohmann::json *errorMessage) const;
    bool TestCondition(const CommandArguments &arguments, size_t index, const CommandContext &context, int *count,
                       nlohmann::json *errorMessage) const;
    bool FindEntities(const std::string &text, const CommandContext &context, std::vector<Entity *> *entities, nlohmann::json *errorMessage) const;
    void FindRelatedEntities(const std::string &relation, Entity *entity, std::vector<Entity *> *entities) const;
    bool ParseRotation(const CommandArguments &arguments, size_t index, const CommandContext &context, float *yaw, float *pitch,
                       nlohmann::json *errorMessage) const;
    bool ParseDimension(const std::string &text, CommandContext *context, ServerLevel **level, nlohmann::json *errorMessage) const;
    bool IsEntityAlive(const CommandContext &context) const;

    CommandDispatcher *m_dispatcher;
};
