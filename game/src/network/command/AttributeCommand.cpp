#include "AttributeCommand.h"

#include <cstdio>

#include "../../core/Localization.h"
#include "../../core/ResourceLocation.h"
#include "../../utils/StringUtils.h"
#include "CommandArguments.h"
#include "CommandContext.h"
#include "CommandSuggestions.h"
#include "CommandTargetResolver.h"

AttributeCommand::AttributeCommand() : Command("attribute")
{}

AttributeCommand::~AttributeCommand()
{}

bool AttributeCommand::RequiresOperator() const
{
    return true;
}

void AttributeCommand::Execute(CommandContext *context, const CommandArguments &arguments)
{
    if (arguments.Count() < 4)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.attribute.usage"), true);
        return;
    }

    CommandTargetResolver resolver(context->GetPlayerList());
    std::vector<ServerPlayer *> targets;
    nlohmann::json errorMessage;
    if (!resolver.ResolvePlayers(arguments.Get(1), context, &targets, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return;
    }

    if (targets.size() != 1)
    {
        context->SendSystemMessage(CommandContext::Translate("argument.entity.toomany"), true);
        return;
    }

    ServerPlayer *target       = targets[0];
    std::wstring targetName    = context->GetPlayerDisplayName(target);
    const Attribute *attribute = Attribute::ByName(StringUtils::ToLower(arguments.Get(2)));
    if (!attribute)
    {
        context->SendSystemMessage(
                CommandContext::Translate("commands.attribute.failed.no_attribute", {targetName, StringUtils::ToUtf16(arguments.Get(2))}), true);
        return;
    }

    AttributeInstance *instance = target->GetAttributes()->Get(*attribute);
    std::wstring attributeName  = GetAttributeName(instance);
    std::string action          = StringUtils::ToLower(arguments.Get(3));
    double scale                = 1.0;
    if (action == "get" && arguments.Count() <= 5)
    {
        if (arguments.Count() == 5 && !arguments.GetDouble(4, &scale, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        context->SendSystemMessage(
                CommandContext::Translate("commands.attribute.value.get.success", {attributeName, targetName, FormatValue(instance->GetValue())}),
                false);
        return;
    }

    std::string subAction = arguments.Count() >= 5 ? StringUtils::ToLower(arguments.Get(4)) : "";
    if (action == "base" && subAction == "get" && arguments.Count() <= 6)
    {
        if (arguments.Count() == 6 && !arguments.GetDouble(5, &scale, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        context->SendSystemMessage(CommandContext::Translate("commands.attribute.base_value.get.success",
                                                             {attributeName, targetName, FormatValue(instance->GetBaseValue())}),
                                   false);
        return;
    }

    if (action == "base" && subAction == "set" && arguments.Count() == 6)
    {
        double value = 0.0;
        if (!arguments.GetDouble(5, &value, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return;
        }

        instance->SetBaseValue(value);
        context->GetPlayerList()->SendAttributesTo(target);
        context->SendSystemMessage(
                CommandContext::Translate("commands.attribute.base_value.set.success", {attributeName, targetName, FormatValue(value)}), false);
        return;
    }

    if (action == "base" && subAction == "reset" && arguments.Count() == 5)
    {
        instance->SetBaseValue(attribute->GetDefaultValue());
        context->GetPlayerList()->SendAttributesTo(target);
        context->SendSystemMessage(CommandContext::Translate("commands.attribute.base_value.reset.success",
                                                             {attributeName, targetName, FormatValue(attribute->GetDefaultValue())}),
                                   false);
        return;
    }

    if (action == "modifier" && ExecuteModifier(context, arguments, target, instance))
    {
        return;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.attribute.usage"), true);
}

bool AttributeCommand::ExecuteModifier(CommandContext *context, const CommandArguments &arguments, ServerPlayer *target, AttributeInstance *instance)
{
    nlohmann::json errorMessage;
    std::wstring targetName    = context->GetPlayerDisplayName(target);
    std::wstring attributeName = GetAttributeName(instance);
    std::string subAction      = arguments.Count() >= 5 ? StringUtils::ToLower(arguments.Get(4)) : "";
    if (subAction == "add" && arguments.Count() == 8)
    {
        AttributeModifier modifier{ResourceLocation(StringUtils::ToLower(arguments.Get(5))).ToString(), 0.0, eAttributeOperation_AddValue};
        if (!arguments.GetDouble(6, &modifier.amount, &errorMessage))
        {
            context->SendSystemMessage(errorMessage, true);
            return true;
        }

        if (!AttributeInstance::ParseOperation(StringUtils::ToLower(arguments.Get(7)), &modifier.operation))
        {
            return false;
        }

        std::wstring modifierName = StringUtils::ToUtf16(modifier.id);
        if (!instance->AddModifier(modifier))
        {
            context->SendSystemMessage(
                    CommandContext::Translate("commands.attribute.failed.modifier_already_present", {modifierName, attributeName, targetName}), true);
            return true;
        }

        context->GetPlayerList()->SendAttributesTo(target);
        context->SendSystemMessage(CommandContext::Translate("commands.attribute.modifier.add.success", {modifierName, attributeName, targetName}),
                                   false);
        return true;
    }

    bool remove   = subAction == "remove" && arguments.Count() == 6;
    bool valueGet = subAction == "value" && arguments.Count() >= 7 && arguments.Count() <= 8 && StringUtils::ToLower(arguments.Get(5)) == "get";
    if (!remove && !valueGet)
    {
        return false;
    }

    std::string id                    = ResourceLocation(StringUtils::ToLower(arguments.Get(remove ? 5 : 6))).ToString();
    std::wstring modifierName         = StringUtils::ToUtf16(id);
    const AttributeModifier *modifier = instance->FindModifier(id);
    if (!modifier)
    {
        context->SendSystemMessage(CommandContext::Translate("commands.attribute.failed.no_modifier", {attributeName, targetName, modifierName}),
                                   true);
        return true;
    }

    if (remove)
    {
        instance->RemoveModifier(id);
        context->GetPlayerList()->SendAttributesTo(target);
        context->SendSystemMessage(CommandContext::Translate("commands.attribute.modifier.remove.success", {modifierName, attributeName, targetName}),
                                   false);
        return true;
    }

    double scale = 1.0;
    if (arguments.Count() == 8 && !arguments.GetDouble(7, &scale, &errorMessage))
    {
        context->SendSystemMessage(errorMessage, true);
        return true;
    }

    context->SendSystemMessage(CommandContext::Translate("commands.attribute.modifier.value.get.success",
                                                         {modifierName, attributeName, targetName, FormatValue(modifier->amount)}),
                               false);
    return true;
}

void AttributeCommand::Complete(CommandContext *context, const std::vector<std::string> &arguments, const BlockPos *target,
                                std::vector<std::string> *completions) const
{
    (void) target;

    CommandSuggestions suggestions(arguments, completions);
    std::string action    = arguments.size() > 2 ? StringUtils::ToLower(arguments[2]) : "";
    std::string subAction = arguments.size() > 3 ? StringUtils::ToLower(arguments[3]) : "";
    if (arguments.size() == 1)
    {
        suggestions.AddPlayers(context->GetPlayerList());
    }
    else if (arguments.size() == 2)
    {
        const std::vector<const Attribute *> &attributes = Attribute::GetAll();
        for (size_t i = 0; i < attributes.size(); i++)
        {
            suggestions.AddMatching(attributes[i]->GetResourceLocation().ToString());
        }
    }
    else if (arguments.size() == 3)
    {
        suggestions.AddMatching("get");
        suggestions.AddMatching("base");
        suggestions.AddMatching("modifier");
    }
    else if (arguments.size() == 4 && action == "base")
    {
        suggestions.AddMatching("get");
        suggestions.AddMatching("set");
        suggestions.AddMatching("reset");
    }
    else if (arguments.size() == 4 && action == "modifier")
    {
        suggestions.AddMatching("add");
        suggestions.AddMatching("remove");
        suggestions.AddMatching("value");
    }
    else if (arguments.size() == 5 && action == "modifier" && subAction == "value")
    {
        suggestions.AddMatching("get");
    }
    else if (arguments.size() == 7 && action == "modifier" && subAction == "add")
    {
        for (uint8_t i = 0; i <= eAttributeOperation_AddMultipliedTotal; i++)
        {
            suggestions.AddMatching(AttributeInstance::GetOperationName((eATTRIBUTE_OPERATION) i));
        }
    }
    else if ((arguments.size() == 5 && action == "modifier" && subAction == "remove") ||
             (arguments.size() == 6 && action == "modifier" && subAction == "value"))
    {
        CommandTargetResolver resolver(context->GetPlayerList());
        std::vector<ServerPlayer *> targets;
        nlohmann::json errorMessage;
        const Attribute *attribute = Attribute::ByName(StringUtils::ToLower(arguments[1]));
        if (attribute && resolver.ResolvePlayers(arguments[0], context, &targets, &errorMessage) && targets.size() == 1)
        {
            const std::vector<AttributeModifier> &modifiers = targets[0]->GetAttributes()->Get(*attribute)->GetModifiers();
            for (size_t i = 0; i < modifiers.size(); i++)
            {
                suggestions.AddMatching(modifiers[i].id);
            }
        }
    }
}

std::wstring AttributeCommand::FormatValue(double value)
{
    char text[64];
    snprintf(text, sizeof(text), "%.15g", value);
    return StringUtils::ToUtf16(text);
}

std::wstring AttributeCommand::GetAttributeName(const AttributeInstance *instance)
{
    return Localization::GetInstance()->Get(instance->GetAttribute()->GetDescriptionId());
}
