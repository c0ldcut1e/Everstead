#include "SoundRegistry.h"

#include <algorithm>
#include <string>
#include <utility>

SoundRegistry::SoundRegistry()
{}

SoundRegistry *SoundRegistry::GetInstance()
{
    static SoundRegistry instance;
    return &instance;
}

void SoundRegistry::Init()
{
    Clear();

    File back("assets/sound/ui/back.ogg");
    File focus("assets/sound/ui/focus.ogg");
    File press("assets/sound/ui/press.ogg");
    File scroll("assets/sound/ui/scroll.ogg");
    AddSound("ui.back", &back);
    AddSound("ui.focus", &focus);
    AddSound("ui.press", &press);
    AddSound("ui.scroll", &scroll);

    File stone1("assets/sound/dig/stone1.ogg");
    File stone2("assets/sound/dig/stone2.ogg");
    File stone3("assets/sound/dig/stone3.ogg");
    File stone4("assets/sound/dig/stone4.ogg");
    AddSound("dig.stone1", &stone1);
    AddSound("dig.stone2", &stone2);
    AddSound("dig.stone3", &stone3);
    AddSound("dig.stone4", &stone4);

    File grass1("assets/sound/dig/grass1.ogg");
    File grass2("assets/sound/dig/grass2.ogg");
    File grass3("assets/sound/dig/grass3.ogg");
    File grass4("assets/sound/dig/grass4.ogg");
    AddSound("dig.grass1", &grass1);
    AddSound("dig.grass2", &grass2);
    AddSound("dig.grass3", &grass3);
    AddSound("dig.grass4", &grass4);

    File snow1("assets/sound/dig/snow1.ogg");
    File snow2("assets/sound/dig/snow2.ogg");
    File snow3("assets/sound/dig/snow3.ogg");
    File snow4("assets/sound/dig/snow4.ogg");
    AddSound("dig.snow1", &snow1);
    AddSound("dig.snow2", &snow2);
    AddSound("dig.snow3", &snow3);
    AddSound("dig.snow4", &snow4);

    File sand1("assets/sound/dig/sand1.ogg");
    File sand2("assets/sound/dig/sand2.ogg");
    File sand3("assets/sound/dig/sand3.ogg");
    File sand4("assets/sound/dig/sand4.ogg");
    AddSound("dig.sand1", &sand1);
    AddSound("dig.sand2", &sand2);
    AddSound("dig.sand3", &sand3);
    AddSound("dig.sand4", &sand4);

    File dirt1("assets/sound/dig/dirt1.ogg");
    File dirt2("assets/sound/dig/dirt2.ogg");
    File dirt3("assets/sound/dig/dirt3.ogg");
    File dirt4("assets/sound/dig/dirt4.ogg");
    AddSound("dig.dirt1", &dirt1);
    AddSound("dig.dirt2", &dirt2);
    AddSound("dig.dirt3", &dirt3);
    AddSound("dig.dirt4", &dirt4);

    File wood1("assets/sound/dig/wood1.ogg");
    File wood2("assets/sound/dig/wood2.ogg");
    File wood3("assets/sound/dig/wood3.ogg");
    File wood4("assets/sound/dig/wood4.ogg");
    AddSound("dig.wood1", &wood1);
    AddSound("dig.wood2", &wood2);
    AddSound("dig.wood3", &wood3);
    AddSound("dig.wood4", &wood4);

    File glass1("assets/sound/random/glass1.ogg");
    File glass2("assets/sound/random/glass2.ogg");
    File glass3("assets/sound/random/glass3.ogg");
    AddSound("random.glass1", &glass1);
    AddSound("random.glass2", &glass2);
    AddSound("random.glass3", &glass3);

    File cloth1("assets/sound/dig/cloth1.ogg");
    File cloth2("assets/sound/dig/cloth2.ogg");
    File cloth3("assets/sound/dig/cloth3.ogg");
    File cloth4("assets/sound/dig/cloth4.ogg");
    AddSound("dig.cloth1", &cloth1);
    AddSound("dig.cloth2", &cloth2);
    AddSound("dig.cloth3", &cloth3);
    AddSound("dig.cloth4", &cloth4);

    File stepCloth1("assets/sound/step/cloth1.ogg");
    File stepCloth2("assets/sound/step/cloth2.ogg");
    File stepCloth3("assets/sound/step/cloth3.ogg");
    File stepCloth4("assets/sound/step/cloth4.ogg");
    AddSound("step.cloth1", &stepCloth1);
    AddSound("step.cloth2", &stepCloth2);
    AddSound("step.cloth3", &stepCloth3);
    AddSound("step.cloth4", &stepCloth4);

    File stepStone1("assets/sound/step/stone1.ogg");
    File stepStone2("assets/sound/step/stone2.ogg");
    File stepStone3("assets/sound/step/stone3.ogg");
    File stepStone4("assets/sound/step/stone4.ogg");
    File stepStone5("assets/sound/step/stone5.ogg");
    File stepStone6("assets/sound/step/stone6.ogg");
    AddSound("step.stone1", &stepStone1);
    AddSound("step.stone2", &stepStone2);
    AddSound("step.stone3", &stepStone3);
    AddSound("step.stone4", &stepStone4);
    AddSound("step.stone5", &stepStone5);
    AddSound("step.stone6", &stepStone6);

    File splash("assets/sound/random/splash.ogg");
    File liquidSplash("assets/sound/liquid/splash.ogg");
    File liquidWater("assets/sound/liquid/water.ogg");
    AddSound("random.splash", &splash);
    AddSound("liquid.splash", &liquidSplash);
    AddSound("liquid.water", &liquidWater);

    File liquidLava("assets/sound/liquid/lava.ogg");
    File liquidLavaPop("assets/sound/liquid/lavapop.ogg");
    File fizz("assets/sound/random/fizz.ogg");
    AddSound("liquid.lava", &liquidLava);
    AddSound("liquid.lavapop", &liquidLavaPop);
    AddSound("random.fizz", &fizz);
    File pop("assets/sound/random/pop.ogg");
    AddSound("random.pop", &pop);
    File orb("assets/sound/random/orb.ogg");
    AddSound("random.orb", &orb);
    File levelUp("assets/sound/random/levelup.ogg");
    AddSound("random.levelup", &levelUp);
    File burp("assets/sound/random/burp.ogg");
    AddSound("random.burp", &burp);
    File pistonOut("assets/sound/tile/piston/out.ogg");
    File pistonIn("assets/sound/tile/piston/in.ogg");
    AddSound("tile.piston.out", &pistonOut);
    AddSound("tile.piston.in", &pistonIn);
    File breakSound("assets/sound/random/break.ogg");
    AddSound("random.break", &breakSound);
    File anvilBreak("assets/sound/random/anvil_break.ogg");
    File anvilLand("assets/sound/random/anvil_land.ogg");
    File anvilUse("assets/sound/random/anvil_use.ogg");
    AddSound("random.anvil_break", &anvilBreak);
    AddSound("random.anvil_land", &anvilLand);
    AddSound("random.anvil_use", &anvilUse);
    File doorOpen("assets/sound/random/door_open.ogg");
    File doorClose("assets/sound/random/door_close.ogg");
    AddSound("random.door_open", &doorOpen);
    AddSound("random.door_close", &doorClose);

    File chestOpen("assets/sound/random/chestopen.ogg");
    File chestClosed("assets/sound/random/chestclosed.ogg");
    AddSound("random.chestopen", &chestOpen);
    AddSound("random.chestclosed", &chestClosed);

    File fire("assets/sound/fire/fire.ogg");
    File fireIgnite("assets/sound/fire/ignite.ogg");
    AddSound("fire.fire", &fire);
    AddSound("fire.ignite1", &fireIgnite);

    File click("assets/sound/random/click.ogg");
    AddSound("random.click", &click);

    File fuse("assets/sound/random/fuse.ogg");
    AddSound("random.fuse", &fuse);
    File bow("assets/sound/random/bow.ogg");
    AddSound("random.bow", &bow);
    for (int variant = 1; variant <= 4; variant++)
    {
        File bowHit("assets/sound/random/bowhit" + std::to_string(variant) + ".ogg");
        AddSound("random.bowhit" + std::to_string(variant), &bowHit);
    }

    const std::pair<const char *, int> stepSounds[] = {{"grass", 6}, {"gravel", 4}, {"sand", 5}, {"wood", 6}, {"snow", 4}, {"ladder", 5}};
    for (const std::pair<const char *, int> &stepSound : stepSounds)
    {
        for (int variant = 1; variant <= stepSound.second; variant++)
        {
            std::string name = std::string(stepSound.first) + std::to_string(variant);
            File file("assets/sound/step/" + name + ".ogg");
            AddSound("step." + name, &file);
        }
    }

    const std::pair<const char *, int> weatherSounds[] = {{"ambient/weather/rain", 4},
                                                          {"ambient/weather/snow", 4},
                                                          {"ambient/weather/thunder", 3},
                                                          {"ambient/cave/cave", 15},
                                                          {"random/explode", 4},
                                                          {"random/eat", 3},
                                                          {"damage/fallbig", 2},
                                                          {"damage/hurtflesh", 3},
                                                          {"mob/pig/say", 3},
                                                          {"mob/cow/say", 4},
                                                          {"mob/cow/hurt", 3},
                                                          {"mob/sheep/say", 3},
                                                          {"mob/chicken/say", 3},
                                                          {"mob/chicken/hurt", 2},
                                                          {"mob/zombie/say", 3},
                                                          {"mob/zombie/hurt", 2},
                                                          {"mob/skeleton/say", 3},
                                                          {"mob/skeleton/hurt", 4},
                                                          {"mob/spider/say", 4},
                                                          {"mob/creeper/say", 4},
                                                          {"mob/slime/say", 5},
                                                          {"mob/slime/attack", 2},
                                                          {"mob/slime/big", 4},
                                                          {"dripstone/break", 5},
                                                          {"dripstone/step", 6},
                                                          {"pointed_dripstone/drip_water", 15},
                                                          {"pointed_dripstone/drip_lava", 6},
                                                          {"pointed_dripstone/land", 5},
                                                          {"bamboo/place", 6},
                                                          {"moss/break", 4},
                                                          {"moss/step", 5},
                                                          {"bamboo/step", 6},
                                                          {"bamboo/sapling_place", 6},
                                                          {"scaffolding/place", 4},
                                                          {"scaffolding/step", 7},
                                                          {"mob/parrot/idle", 6},
                                                          {"mob/parrot/hurt", 2},
                                                          {"mob/parrot/death", 4},
                                                          {"mob/ocelot/idle", 4},
                                                          {"mob/ocelot/hurt", 3},
                                                          {"mob/ocelot/death", 3},
                                                          {"leashknot/place", 1},
                                                          {"leashknot/break", 2},
                                                          {"basalt/break", 5},
                                                          {"basalt/step", 6},
                                                          {"chain/break", 4},
                                                          {"chain/step", 6},
                                                          {"lantern/break", 6},
                                                          {"lantern/place", 6},
                                                          {"cherry_wood/break", 5},
                                                          {"cherry_wood/step", 6},
                                                          {"cherry_leaves/break", 5},
                                                          {"cherry_leaves/step", 5},
                                                          {"bamboo_wood/break", 5},
                                                          {"bamboo_wood/step", 6},
                                                          {"sign/waxed_interact_fail", 3},
                                                          {"dye/dye", 1},
                                                          {"ink_sac/ink_sac", 3},
                                                          {"honeycomb/wax_on", 3},
                                                          {"fracture/resonance", 3}};
    for (const std::pair<const char *, int> &weatherSound : weatherSounds)
    {
        for (int variant = 1; variant <= weatherSound.second; variant++)
        {
            std::string path = std::string(weatherSound.first) + std::to_string(variant);
            File file("assets/sound/" + path + ".ogg");
            std::replace(path.begin(), path.end(), '/', '.');
            AddSound(path, &file);
        }
    }

    File fallSmall("assets/sound/damage/fallsmall.ogg");
    AddSound("damage.fallsmall", &fallSmall);
    File pigDeath("assets/sound/mob/pig/death.ogg");
    AddSound("mob.pig.death", &pigDeath);
    File chickenPlop("assets/sound/mob/chicken/plop.ogg");
    AddSound("mob.chicken.plop", &chickenPlop);
    File zombieDeath("assets/sound/mob/zombie/death.ogg");
    AddSound("mob.zombie.death", &zombieDeath);
    File skeletonDeath("assets/sound/mob/skeleton/death.ogg");
    AddSound("mob.skeleton.death", &skeletonDeath);
    File spiderDeath("assets/sound/mob/spider/death.ogg");
    AddSound("mob.spider.death", &spiderDeath);
    File creeperDeath("assets/sound/mob/creeper/death.ogg");
    AddSound("mob.creeper.death", &creeperDeath);
    File drink("assets/sound/random/drink.ogg");
    AddSound("random.drink", &drink);
    constexpr const char *kNoteSounds[] = {"harp",   "bd",       "bass",     "snare",          "hat",      "pling",      "bell", "flute",
                                           "guitar", "icechime", "xylobone", "iron_xylophone", "cow_bell", "didgeridoo", "bit",  "banjo"};
    for (const char *noteSound : kNoteSounds)
    {
        File file("assets/sound/note/" + std::string(noteSound) + ".ogg");
        AddSound("note." + std::string(noteSound), &file);
    }

    File resonantHum("assets/sound/ambient/resonant_hum.ogg");
    AddSound("ambient.resonant_hum", &resonantHum);
    File resonantSocketHum("assets/sound/ambient/resonant_socket_hum.ogg");
    AddSound("ambient.resonant_socket_hum", &resonantSocketHum);
    File fractureOpen("assets/sound/fracture/open.ogg");
    AddSound("fracture.open", &fractureOpen);
    File fractureClose("assets/sound/fracture/close.ogg");
    AddSound("fracture.close", &fractureClose);
    File fractureStabilize("assets/sound/fracture/stabilize.ogg");
    AddSound("fracture.stabilize", &fractureStabilize);
    File fractureCollapse("assets/sound/fracture/collapse.ogg");
    AddSound("fracture.collapse", &fractureCollapse);
    File fracturePhase("assets/sound/fracture/phase.ogg");
    AddSound("fracture.phase", &fracturePhase);
    File fractureEchoing("assets/sound/fracture/echoing.ogg");
    AddSound("fracture.echoing", &fractureEchoing);
    File fractureEchoingFail("assets/sound/fracture/echoing_fail.ogg");
    AddSound("fracture.echoing_fail", &fractureEchoingFail);
    File fractureSplinter("assets/sound/fracture/splinter.ogg");
    AddSound("fracture.splinter", &fractureSplinter);
    File fractureAfterimage("assets/sound/fracture/afterimage.ogg");
    AddSound("fracture.afterimage", &fractureAfterimage);
    File fractureFaultLine("assets/sound/fracture/fault_line.ogg");
    AddSound("fracture.fault_line", &fractureFaultLine);
    File fractureUndertow("assets/sound/fracture/undertow.ogg");
    AddSound("fracture.undertow", &fractureUndertow);
    File fractureShatterpoint("assets/sound/fracture/shatterpoint.ogg");
    AddSound("fracture.shatterpoint", &fractureShatterpoint);
    File fractureSeam("assets/sound/fracture/seam.ogg");
    AddSound("fracture.seam", &fractureSeam);
    File fractureRefraction("assets/sound/fracture/refraction.ogg");
    AddSound("fracture.refraction", &fractureRefraction);
    File fractureRicochet("assets/sound/fracture/ricochet.ogg");
    AddSound("fracture.ricochet", &fractureRicochet);
    File fractureRecall("assets/sound/fracture/recall.ogg");
    AddSound("fracture.recall", &fractureRecall);
    File fractureTear("assets/sound/fracture/tear.ogg");
    AddSound("fracture.tear", &fractureTear);
    File experimentSequence("assets/sound/experiment/sequence.ogg");
    AddSound("experiment.sequence", &experimentSequence);
    File experimentSingularity("assets/sound/experiment/singularity.ogg");
    AddSound("experiment.singularity", &experimentSingularity);
    File experimentNova("assets/sound/experiment/nova.ogg");
    AddSound("experiment.nova", &experimentNova);
    File experimentTempest("assets/sound/experiment/tempest.ogg");
    AddSound("experiment.tempest", &experimentTempest);
    File experimentPrism("assets/sound/experiment/prism.ogg");
    AddSound("experiment.prism", &experimentPrism);
    File experimentHellgate("assets/sound/experiment/hellgate.ogg");
    AddSound("experiment.hellgate", &experimentHellgate);
    File echoSeedInfuse("assets/sound/echo_seed/infuse.ogg");
    AddSound("echo_seed.infuse", &echoSeedInfuse);
    File resonatorFracture("assets/sound/resonator/fracture.ogg");
    AddSound("resonator.fracture", &resonatorFracture);
    File resonatorStabilize("assets/sound/resonator/stabilize.ogg");
    AddSound("resonator.stabilize", &resonatorStabilize);

    File voidAmbience("assets/sound/ambient/void.ogg");
    GetInstance()->m_music["void"].push_back(AddStreamingSound("ambient.void", &voidAmbience));

    const char *const musicFolders[] = {"menu", "game", "cave", "ice_caves", "desert_caves"};
    for (const char *folder : musicFolders)
    {
        std::string folderName(folder);
        std::vector<SoundEvent *> &tracks = GetInstance()->m_music[folderName];
        for (const File &file : File("assets/sound/music/" + folderName).ListFiles())
        {
            if (!file.IsRegularFile() || file.GetExtension() != ".ogg")
            {
                continue;
            }

            std::string trackName = folderName + "." + file.GetName().substr(0, file.GetName().size() - 4);
            File trackFile(file.GetPath());
            SoundEvent *track = AddMusic("music." + trackName, &trackFile);
            if (track)
            {
                tracks.push_back(track);
            }
        }
    }
}

void SoundRegistry::Clear()
{
    GetInstance()->ClearRegistry();
}

SoundEvent *SoundRegistry::AddSound(const ResourceLocation &location, File *file)
{
    return GetInstance()->RegisterEvent(location, file, eSoundEventType_Sound);
}

SoundEvent *SoundRegistry::AddMusic(const ResourceLocation &location, File *file)
{
    return GetInstance()->RegisterEvent(location, file, eSoundEventType_Music);
}

SoundEvent *SoundRegistry::AddStreamingSound(const ResourceLocation &location, File *file)
{
    return GetInstance()->RegisterEvent(location, file, eSoundEventType_StreamingSound);
}

const std::vector<SoundEvent *> &SoundRegistry::GetMusic(const std::string &folder)
{
    return GetInstance()->m_music[folder];
}

SoundEvent *SoundRegistry::ByName(const ResourceLocation &location)
{
    SoundRegistry *registry = GetInstance();
    auto it                 = registry->m_locationToId.find(location);
    if (it == registry->m_locationToId.end())
    {
        return nullptr;
    }

    return ById(it->second);
}

SoundEvent *SoundRegistry::ById(uint32_t id)
{
    SoundRegistry *registry = GetInstance();
    if (id >= (uint32_t) registry->m_events.size())
    {
        return nullptr;
    }

    return registry->m_events[id];
}

bool SoundRegistry::Has(const ResourceLocation &location)
{
    SoundRegistry *registry = GetInstance();
    return registry->m_locationToId.find(location) != registry->m_locationToId.end();
}

uint32_t SoundRegistry::Size()
{
    return (uint32_t) GetInstance()->m_events.size();
}

SoundEvent *SoundRegistry::RegisterEvent(const ResourceLocation &location, File *file, eSOUND_EVENT_TYPE type)
{
    if (!file)
    {
        return nullptr;
    }

    SoundEvent *existingEvent = ByName(location);
    if (existingEvent)
    {
        return existingEvent;
    }

    std::unique_ptr<SoundEvent> event = std::make_unique<SoundEvent>(location, *file, type);
    SoundEvent *eventPtr              = event.get();
    uint32_t id                       = (uint32_t) m_events.size();

    m_locationToId[location] = id;
    m_events.push_back(eventPtr);
    m_ownedEvents.push_back(std::move(event));

    return eventPtr;
}

void SoundRegistry::ClearRegistry()
{
    m_ownedEvents.clear();
    m_events.clear();
    m_locationToId.clear();
    m_music.clear();
}
