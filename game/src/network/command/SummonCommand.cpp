#include "SummonCommand.h"

#include "../../entity/Entity.h"
#include "../../entity/EntityFactory.h"
#include "../../tag/TagParser.h"
#include "../../utils/Random.h"
#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandPositionParser.h"
#include "CommandSuggestions.h"

SummonCommand::SummonCommand() : Command("summon")
{}

SummonCommand::~SummonCommand()
{}

bool SummonCommand::RequiresOperator() const
{
    return true;
}

void SummonCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 2 || arguments.Count() == 3 || arguments.Count() == 4)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.summon.usage"), true);
        return;
    }

    ResourceLocation location;
    eENTITY_TYPE type;
    if (!ResourceLocation::TryParse(StringUtils::ToLower(arguments.Get(1)), &location) || !EntityFactory::ByName(location, &type))
    {
        context->SendSystemMessage(CommandContext::Translate("commands.summon.unknown", {StringUtils::ToUtf16(arguments.Get(1))}), true);
        return;
    }

    glm::dvec3 position = context->GetPosition();
    if (arguments.Count() >= 5)
    {
        CommandPositionParser positionParser;
        nlohmann::json errorMessage;
        if (!positionParser.ParseVec3(arguments.GetValues(), 2, context, &position, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }
    }

    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    if (arguments.Count() > 5)
    {
        TagParser parser(arguments.GetRest(5));
        tag = parser.Parse();
        if (!tag)
        {
            context->SendSystemMessage(CommandContext::Translate("argument.nbt.invalid", {std::to_wstring(parser.GetPosition())}), true);
            return;
        }
    }

    std::unique_ptr<Entity> entity = EntityFactory::Create(type, context->GetLevel());
    entity->ReadSpawnData(EntityFactory::CreateSpawnData(type, &context->GetLevel()->GetRandom()));
    entity->Load(tag.get());
    entity->SetPosition(position);
    context->GetLevel()->AddEntity(std::move(entity));
    context->SendSystemMessage(CommandContext::Translate("commands.summon.success", {StringUtils::ToUtf16(location.ToString())}), false);
}

void SummonCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                             std::vector<std::string> *completions) const
{
    (void) context;

    CommandSuggestions suggestions(arguments, completions);
    if (arguments.size() == 1)
    {
        std::vector<std::string> names;
        EntityFactory::GetSpawnableNames(&names);
        for (size_t i = 0; i < names.size(); i++)
        {
            suggestions.AddMatching(names[i]);
        }
    }

    suggestions.AddCoordinate(1, target);
}
