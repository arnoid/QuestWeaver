//
// Created by michael on 10.08.15.
//

#include <World/WorldModel.h>

using namespace weave;
using namespace std;

ID WorldModel::NewId() {
    idGenerator++;
    return idGenerator;
}

WorldModel::WorldModel() noexcept {
    rs = make_shared<RandomStream>(0);
    actionHistory.reserve(10000);
}

void WorldModel::Execute(vector<WorldModelAction> modelActions) {
    for (auto action : modelActions) {
        ID id = action.GetEntity()->GetId();
        auto entity = GetEntityById(id);
        if (entity.get() == nullptr && action.GetActionType() != WorldActionType::CREATE) {
            Logger::Error(ContractFailedException(
                    "Unable to execute model action: entity with id " + to_string(id) + " not found (type " +
                    action.GetEntity()->GetType() + ")."));
            continue;
        }
        if (action.GetActionType() == WorldActionType::KEEP) {
            Logger::Debug("WorldModel: Keep entity " + to_string(id) + " (" + action.GetEntity()->GetType() + ")", 2);
        } else if (action.GetActionType() == WorldActionType::CREATE) {
            if (id != WorldEntity::NoID) {
                Logger::Error(ContractFailedException(
                        "Unable to execute model action create: entity already has an ID: " + to_string(id)));
                continue;
            }

            ID newId = NewId();
            action.GetEntity()->id = newId;
            entities[newId] = action.GetEntity();
            updateMetaDataForId(newId, action.GetMetaData());
            Logger::Debug("WorldModel: Create entity " + to_string(newId) + " (" + action.GetEntity()->GetType() + ")",
                          2);
        } else if (action.GetActionType() == WorldActionType::DELETE) {
            Logger::Debug("WorldModel: Delete entity " + to_string(id) + " (" + action.GetEntity()->GetType() + ")", 2);
            action.GetEntity()->id = WorldEntity::NoID;
            entities.erase(id);
        } else if (action.GetActionType() == WorldActionType::UPDATE) {
            Logger::Debug("WorldModel: Update entity " + to_string(id) + " (" + action.GetEntity()->GetType() + ")", 2);
            updateMetaDataForId(id, action.GetMetaData());
        } else {
            Logger::Error(ContractFailedException("Illegal action type."));
            continue;
        }

        // update change history
        if (action.GetActionType() != WorldActionType::KEEP) {
            actionHistory.push_back(action);
        }
    }
    if (!modelActions.empty()) {
        entityTypeCache.clear();
        informListeners(modelActions);
    }
}

void WorldModel::updateMetaDataForId(ID newId, const MetaData &newData) {
    for (string name : newData.GetValueNames()) {
        metaData[newId].SetValue(name, newData.GetValue(name));
    }
}

shared_ptr<WorldEntity> WorldModel::GetEntityById(ID id) const noexcept {
    return hasEntityWithId(id) ? entities.find(id)->second : shared_ptr<WorldEntity>();
}

vector<shared_ptr<WorldEntity>> WorldModel::GetEntitiesWithType(string type) const noexcept {
    if (entityTypeCache.empty()) {
        for (auto &mapEntry : entities) {
            auto entity = mapEntry.second;
            entityTypeCache[entity->GetType()].push_back(entity);
        }
    }
    return entityTypeCache[type];
}

bool WorldModel::hasEntityWithId(ID id) const noexcept {
    return id != 0 && entities.count(id) > 0;
}

vector<shared_ptr<WorldEntity>> WorldModel::GetEntities() const noexcept {
    vector<shared_ptr<WorldEntity>> result;
    for (auto pair : entities) {
        result.push_back(pair.second);
    }
    return result;
}

MetaData WorldModel::GetMetaData(ID entityId) const noexcept {
    auto mapEntry = metaData.find(entityId);
    return mapEntry == metaData.end() ? MetaData() : mapEntry->second;
}

vector<WorldModelAction> WorldModel::GetMetaDataHistoryForId(ID id) const noexcept {
    vector<WorldModelAction> result;
    for (auto action : actionHistory) {
        if (action.GetEntity()->GetId() == id &&
            (action.GetActionType() == WorldActionType::CREATE || action.GetActionType() == WorldActionType::UPDATE) &&
            action.GetMetaData().GetValueNames().size() > 0) {
            result.push_back(action);
        }
    }
    return result;
}

vector<WorldModelAction> WorldModel::GetHistory() const noexcept {
    return actionHistory;
}

void WorldModel::AddListener(shared_ptr<WorldListener> listener) const noexcept {
    if (listener) {
        listeners.push_back(listener);
    }
}

void WorldModel::informListeners(const vector<WorldModelAction> &actions) const {
    for (auto &listener : listeners) {
        listener->WorldChanged(actions);
    }
}

WorldModelAction WorldModel::ChangeMetaData(ID entityId, string key, function<int(int)> updater) const {
    auto entity = GetEntityById(entityId);
    auto metaData = GetMetaData(entityId);
    return WorldModelAction(WorldActionType::UPDATE, entity, MetaData(key, updater(metaData.GetValue(key))));
}
