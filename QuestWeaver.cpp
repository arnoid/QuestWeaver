//
// Created by michael on 10.08.15.
//

#include <algorithm>
#include <QuestWeaver.h>
#include <World/Space/SpaceWorldModel.h>

using namespace std;
using namespace weave;

QuestWeaver::QuestWeaver(WeaverConfig &config) {
    if (!config.worldModel) {
        Logger::Fatal("A world model must be provided for the quest system to work.");
    }
    randomStream = config.randomStream ? config.randomStream : make_shared<RandomStream>(config.seed);
    engine.reset(new WeaverEngine(randomStream));
    quests.reset(new QuestModel());
    templates.reset(new TemplateEngine(randomStream, config.dirs, config.formatterType));

    config.worldModel->rs = randomStream;
    world = std::move(config.worldModel);

    stories.reset(new StoryWriter(randomStream, *quests, *templates, *world, config.dirs));
    for (uint64_t i = 0; i < config.questTemplateFactories.size(); i++) {
        RegisterQuestTemplateFactory(move(config.questTemplateFactories.at(i)));
    }
    for (uint64_t i = 0; i < config.storyTemplateFactories.size(); i++) {
        RegisterStoryTemplateFactory(move(config.storyTemplateFactories.at(i)));
    }
}

vector<shared_ptr<Quest>> QuestWeaver::CreateNewQuests() {
    vector<QuestCandidate> newQuestCandidates;
    bool hasPriorityQuests = false;
    int maxScore = 0;
    int minScore = -1;

    // gather quest candidates
    Logger::Debug("Starting new quest creation...");
    for (auto questTemplate : templates->GetTemplatesForNewQuest(*world, *quests)) {
        EngineResult result = engine->fillTemplate(questTemplate, *quests, *world);
        shared_ptr<Quest> newQuest = questTemplate->ToQuest(result.GetQuestPropertyValues());
        int score = 0;  // smaller score is better
        if (questTemplate->HasPriority()) {
            hasPriorityQuests = true;
        } else {
            score += questTemplate->GetRarity();
            for (auto oldQuest : quests->GetQuests()) {
                // penalize quests that occurred often
                if (newQuest->GetType() == oldQuest->GetType()) {
                    score++;
                }
            }
            for (auto action : result.GetModelActions()) {
                // penalize quests that need to create new entities
                if (action.GetActionType() == WorldActionType::CREATE) {
                    score++;
                }
            }
            maxScore = max(maxScore, score);
            minScore = minScore == -1 ? score : min(minScore, score);
        }
        newQuestCandidates.push_back({newQuest, result, score, questTemplate->HasPriority()});
    }

    // shuffle candidates, then sort by score
    auto begin = newQuestCandidates.begin();
    auto end = newQuestCandidates.end();
    for (auto iter = begin + 1; iter != end; iter++) {
        auto iter2 = begin + randomStream->GetRandomIndex((iter - begin) + 1);
        if (iter != iter2) {
            iter_swap(iter, iter2);
        }
    }
    auto comparator = [](const QuestCandidate &a, const QuestCandidate &b) -> bool {
        return a.score < b.score;
    };
    stable_sort(newQuestCandidates.begin(), newQuestCandidates.end(), comparator);

    Logger::Debug("Quest types by absolute score:", 1);
    for (auto candidate : newQuestCandidates) {
        Logger::Debug(to_string(candidate.score) + ": " + candidate.quest->GetType(), 2);
    }

    // randomly select a minimum required score
    vector<shared_ptr<Quest>> newQuests;
    int selectedScore = -1;
    int normalizedScore = maxScore - minScore;  // to prevent the scores getting too big as the game progresses
    while (selectedScore < 0) {
        selectedScore = randomStream->GetNormalIntInRange(-normalizedScore, normalizedScore);
    }

    // select the fitting candidate(s)
    Logger::Debug("Selecting one of the candidates with a score bigger " + to_string(selectedScore + minScore), 1);
    for (auto candidate : newQuestCandidates) {
        Logger::Debug("Processing quest candidate " + candidate.quest->GetType(), 1);
        if (hasPriorityQuests) {
            if (candidate.isPriorityQuest) {
                Logger::Debug("[Priority] quest!", 2);
                world->Execute(candidate.result.GetModelActions());
                Story storyResult = stories->CreateStory(candidate.result.GetStoryParameters());
                world->Execute(storyResult.worldActions);
                quests->RegisterNew(candidate.quest, candidate.result.GetQuestPropertyValues(), storyResult.text);
                newQuests.push_back(candidate.quest);
            } else {
                Logger::Debug("[Ignored] because it is not a priority quest.", 2);
            }
            continue;
        }
        if (candidate.score >= (selectedScore + minScore)) {
            Logger::Debug("Quest was chosen! Updating world model with quest changes...", 2);
            world->Execute(candidate.result.GetModelActions());
            Logger::Debug("Creating story for chosen quest...", 2);
            Story storyResult = stories->CreateStory(candidate.result.GetStoryParameters());
            Logger::Debug("Updating world model with story changes...", 2);
            world->Execute(storyResult.worldActions);
            quests->RegisterNew(candidate.quest, candidate.result.GetQuestPropertyValues(), storyResult.text);
            newQuests.push_back(candidate.quest);
            break;
        }
        Logger::Debug("[Ignored] because its score was too low.", 2);
    }
    Logger::Debug("Quest creation finished, chose " + to_string(newQuests.size()) + " quest(s).", 1);
    return newQuests;
}

std::vector<std::shared_ptr<Quest>> QuestWeaver::GetQuestsWithState(QuestState state) const {
    return quests->GetQuestsWithState(state);
}

void QuestWeaver::Tick(float delta) {
    for (const auto &quest : quests->GetQuestsWithState(QuestState::Active)) {
        const QuestTickResult &change = quest->Tick(delta, *world);
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

bool QuestWeaver::ChangeQuestState(QuestModelAction questAction) {
    Logger::Debug("Directly changing quest " + to_string(questAction.GetQuestId()) + " state to " +
                  to_string(static_cast<int>(questAction.GetActionType())));
    return quests->Execute(questAction);
}

void QuestWeaver::RegisterQuestTemplateFactory(std::unique_ptr<QuestTemplateFactory> factory) {
    templates->RegisterTemplateFactory(move(factory));
}

void QuestWeaver::RegisterStoryTemplateFactory(std::unique_ptr<StoryTemplateFactory> factory) {
    stories->RegisterTemplateFactory(move(factory));
}

void QuestWeaver::Serialize(std::ostream &outputStream, StreamType type) {
    if (type == StreamType::JSON) {
        cereal::JSONOutputArchive outputArchive(outputStream);
        outputArchive(*this);
    } else if (type == StreamType::BINARY) {
        cereal::PortableBinaryOutputArchive outputArchive(outputStream);
        outputArchive(*this);
    } else {
        Logger::Fatal("Unknown serialization type!");
    }
}

QuestWeaver QuestWeaver::Deserialize(std::istream &inputStream, StreamType type) {
    QuestWeaver deserialized;
    if (type == StreamType::JSON) {
        cereal::JSONInputArchive inputArchive(inputStream);
        inputArchive(deserialized);
    } else if (type == StreamType::BINARY) {
        cereal::PortableBinaryInputArchive inputArchive(inputStream);
        inputArchive(deserialized);
    } else {
        Logger::Fatal("Unknown serialization type!");
    }
    return deserialized;
}

QuestWeaver QuestWeaver::Deserialize(std::istream &inputStream, StreamType type, Directories currentDirectories) {
    QuestWeaver deserialized = Deserialize(inputStream, type);
    deserialized.ChangeWorkingDirectories(currentDirectories);
    return deserialized;
}

QuestWeaver::QuestWeaver() {
}

void weave::QuestWeaver::ChangeWorkingDirectories(Directories directories) {
    templates->ChangeDirectories(directories);
    stories->ChangeDirectories(directories);
}

WorldModel &QuestWeaver::GetWorldModel() const {
    return *world;
}
