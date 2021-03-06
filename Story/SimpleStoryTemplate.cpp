//
// Created by michael on 16.02.16.
//

#include <Story/SimpleStoryTemplate.h>

using namespace weave;
using namespace std;

StoryTemplateResult SimpleStoryTemplate::CreateStory(const EntityMap &requiredEntities, const WeaverGraph &,
                                                     const WorldModel &worldModel,
                                                     shared_ptr<RandomStream> randomStream) const {
    auto entities = getValidEntities(requiredEntities, worldModel, randomStream);
    if (entities.empty()) {
        Logger::Fatal("Invalid template call, not all required entities were found!");
    }

    TokenToEntityMap tokenEntityMap;
    StoryTemplateResult result;
    for (uint64_t i = 1; i <= GetRequiredEntities().size(); i++) {
        auto entity = entities[i - 1];
        tokenEntityMap[to_string(i)] = {entity->GetId()};

        if (conditions.count(StoryCondition::OncePerEntity)) {
            result.worldActions.emplace_back(WorldActionType::UPDATE, entity, MetaData(key, 1));
        }
    }

    result.tokenMap = createTokenMapping(tokenEntityMap);
    result.rawText = rawText;
    return result;
}

bool SimpleStoryTemplate::IsValid(const EntityMap &requiredEntities, const WeaverGraph &,
                                  const WorldModel &worldModel) const {
    return getValidEntities(requiredEntities, worldModel, make_shared<RandomStream>()).size() > 0;
}

vector<shared_ptr<WorldEntity>> SimpleStoryTemplate::getValidEntities(const EntityMap &entityMap,
                                                                      const WorldModel &worldModel,
                                                                      shared_ptr<RandomStream> randomStream) const {
    vector<shared_ptr<WorldEntity>> result;
    for (string type : GetRequiredEntities()) {
        vector<shared_ptr<WorldEntity>> validEntities;
        auto mapIter = entityMap.find(type);
        if (mapIter == entityMap.end()) {
            return validEntities;
        }

        for (shared_ptr<WorldEntity> entity : mapIter->second) {
            ID id = entity->GetId();
            MetaData metaData = worldModel.GetMetaData(id);

            // check the "once per entity" condition
            if (conditions.count(StoryCondition::OncePerEntity) && metaData.HasValue(key)) {
                continue;
            }

            // check the "without property" condition
            auto iter = conditions.find(StoryCondition::WithoutProperty);
            if (iter != conditions.end() && !iter->second.empty() && metaData.HasValue(iter->second[0])) {
                continue;
            }

            // check the "with property" condition
            iter = conditions.find(StoryCondition::WithProperty);
            if (iter != conditions.end() && !iter->second.empty() && !metaData.HasValue(iter->second[0])) {
                continue;
            }

            // check the "greater than" condition
            iter = conditions.find(StoryCondition::GreaterThan);
            if (iter != conditions.end() && iter->second.size() == 2) {
                string property = iter->second[0];
                int value = atoi(iter->second[1].c_str());
                if (!metaData.HasValue(property) || metaData.GetValue(property) <= value) {
                    continue;
                }
            }

            // check the "smaller than" condition
            iter = conditions.find(StoryCondition::SmallerThan);
            if (iter != conditions.end() && iter->second.size() == 2) {
                string property = iter->second[0];
                int value = atoi(iter->second[1].c_str());
                if (!metaData.HasValue(property) || metaData.GetValue(property) >= value) {
                    continue;
                }
            }
            validEntities.push_back(entity);
        }
        if (validEntities.empty()) {
            return validEntities;
        }
        result.push_back(validEntities[randomStream->GetRandomIndex(validEntities.size())]);
    }
    return result;
}
