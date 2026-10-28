-- Game classes are globals and their methods are generated from the game's headers, so the editor
-- autocompletes them once workspace.library points at mods/library

-- Hooks wrap game functions. Call original to keep the game's behavior, or skip it to replace it
hook("ItemRegistry::Init", function(original)
    original()

    -- Strings convert to ResourceLocation, so "example:ruby" works anywhere one is expected
    local ruby = new("Item")
    ruby:SetIconPath(mod.directory .. "/ruby.png")
    ruby:SetMaxStackSize(16)
    ruby:SetId(ItemRegistry.GetInstance():Register("example:ruby", ruby))

    local sword = new("Item")
    sword:SetIconPath(mod.directory .. "/ruby.png")
    sword:SetMaxStackSize(1)
    sword:SetMaxDamage(500)
    sword:SetAttackDamage(8)
    sword:SetHandEquipped(true)
    sword:SetId(ItemRegistry.GetInstance():Register("example:ruby_sword", sword))

    -- Vanilla items can be changed the same way
    Item.ByName("stick"):SetMaxStackSize(16)

    log.info("Registered " .. ruby:GetResourceLocation():ToString() .. " as item " .. ruby:GetId())
end)

-- Hooks get the object as self followed by the C++ arguments
hook("Item::SetMaxDamage", function(original, self, maxDamage)
    original(self, maxDamage * 2)
end)
