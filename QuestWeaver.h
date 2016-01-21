//
// Created by michael on 10.08.15.
//

#pragma once

#include "Core/WeaverTypes.h"
#include "cereal.h"
#include "json/json.h"
#include "Core/WeaverEngine.h"
#include "QuestModel/QuestModel.h"
#include "Template/TemplateEngine.h"
#include "World/WorldModel.h"
#include "QuestModel/Quest.h"
#include "Core/WeaverUtils.h"
#include "World/WorldListener.h"
#include "WeaverConfig.h"

namespace weave {
    enum class StreamType {
        JSON, BINARY
    };

    class QuestWeaver {
    public:
        explicit QuestWeaver(uint64_t seed);

        explicit QuestWeaver(WeaverConfig config);

        std::vector<std::shared_ptr<Quest>> GetQuestsWithState(QuestState state) const;

        std::vector<std::shared_ptr<Quest>> GetAllQuests() const;

        std::shared_ptr<Quest> CreateNewQuest();

        std::shared_ptr<Quest> GetQuest(ID questId) const;

        void Tick(float delta);

        void RegisterTemplateFactory(std::shared_ptr<TemplateFactory> factory);

        std::shared_ptr<Quest> ChangeQuestState(QuestModelAction questAction);

        void serialize(std::ostream &outputStream, StreamType type);

        static QuestWeaver deserialize(std::istream &inputStream, StreamType type);

    private:
        std::unique_ptr<WeaverEngine> engine;
        std::unique_ptr<QuestModel> quests;
        std::unique_ptr<TemplateEngine> templates;
        std::unique_ptr<WorldModel> world;
        std::unique_ptr<StoryWriter> stories;
        std::shared_ptr<RandomStream> randomStream;
        Directories dirs;

        // serialization
        friend class cereal::access;

        QuestWeaver();

        template<class Archive>
        void load(Archive &archive) {
            archive(CEREAL_NVP(randomStream), CEREAL_NVP(quests), CEREAL_NVP(world), CEREAL_NVP(dirs));
            engine.reset(new WeaverEngine(randomStream));
            stories.reset(new StoryWriter(randomStream, *quests, *templates));
            templates.reset(new TemplateEngine(randomStream, dirs));
        }

        template<class Archive>
        void save(Archive &archive) const {
            archive(CEREAL_NVP(randomStream), CEREAL_NVP(quests), CEREAL_NVP(world), CEREAL_NVP(dirs));
        }
    };
}
