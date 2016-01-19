//
// Created by michael on 10.08.15.
//

#include "QuestWeaver.h"
#include <algorithm>
#include <iostream>
#include "Template/Space/SpaceQuestTemplateFactory.h"
#include "World/Space/SpaceWorldModel.h"
#include "WeaverConfig.h"

using namespace std;
using namespace weave;

QuestWeaver::QuestWeaver(uint64_t seed) : QuestWeaver(WeaverConfig(seed)) {
}

QuestWeaver::QuestWeaver(WeaverConfig config) {
    randomStream.reset(new RandomStream(config.seed));
    engine.reset(new WeaverEngine(randomStream));
    quests.reset(new QuestModel());
    templates.reset(new TemplateEngine());
    if (config.worldModel != nullptr) {
        world.reset(config.worldModel);
    } else if (config.debug) {
        world.reset(new SpaceWorldModel(randomStream));
    } else {
        throw ContractFailedException("A world model must be provided for the quest system to work.");
    }
    stories.reset(new StoryWriter(randomStream, *quests, *templates));
    if (config.debug) {
        shared_ptr<TemplateFactory> spaceFactory = make_shared<SpaceQuestTemplateFactory>(randomStream, config.dirs);
        templates->RegisterTemplateFactory(spaceFactory);
    } else {
        for (auto factory : config.templateFactories) {
            templates->RegisterTemplateFactory(factory);
        }
    }
}

shared_ptr<Quest> QuestWeaver::CreateNewQuest() {
    auto questTemplate = templates->GetTemplateForNewQuest(randomStream);
    EngineResult result = engine->fillTemplate(questTemplate, *quests, *world, *stories);
    world->Execute(result.GetModelActions());
    shared_ptr<Quest> newQuest = questTemplate->ToQuest(result.GetQuestPropertyValues(), result.GetStory());
    return quests->RegisterNew(newQuest, result.GetQuestPropertyValues());
}

std::vector<std::shared_ptr<Quest>> QuestWeaver::GetQuestsWithState(QuestState state) const {
    return quests->GetQuestsWithState(state);
}

void QuestWeaver::Tick(float delta) {
    for (const auto &quest : quests->GetQuests()) {
        const QuestTickResult &change = quest->Tick(delta);
        world->Execute(change.GetWorldChanges());
        quests->Execute(change.GetQuestChange());
    }
}

std::vector<std::shared_ptr<Quest>> QuestWeaver::GetAllQuests() const {
    return quests->GetQuests();
}

shared_ptr<Quest> QuestWeaver::GetQuest(ID questId) const {
    return quests->GetQuest(questId);
}

std::shared_ptr<Quest> QuestWeaver::ChangeQuestState(QuestModelAction questAction) {
    return quests->Execute(questAction);
}

void QuestWeaver::RegisterTemplateFactory(std::shared_ptr<TemplateFactory> factory) {
    templates->RegisterTemplateFactory(factory);
}
