//
// Created by michael on 11.08.15.
//

#include <QuestModel/Quest.h>

using namespace weave;
using namespace std;

string Quest::GetTitle() const {
    return title;
}

string Quest::GetDescription() const {
    return description;
}

Quest::Quest(const string &title, const string &description) :
        Quest(NoID, title, description, "") {
}

Quest::Quest(ID newId, const string &title, const string &description, const std::string &story) :
        id(newId), title(title), description(description), story(story) {
}


Quest::Quest(const std::string &title, const std::string &description, const std::string &story) :
        Quest(NoID, title, description, story) {
}

ID Quest::GetId() const {
    return id;
}

QuestTickResult Quest::Tick(float, const WorldModel &) {
    vector<WorldModelAction> emptyChanges;
    QuestModelAction emptyAction(QuestActionType::KEEP, id);
    return QuestTickResult(emptyChanges, emptyAction);
}

QuestTickResult::QuestTickResult(ID questId) noexcept :
        questChanges(QuestModelAction(QuestActionType::KEEP, questId)) { }

QuestTickResult::QuestTickResult(vector<WorldModelAction> worldChanges, QuestModelAction questChanges) noexcept :
        worldChanges(worldChanges), questChanges(questChanges) { }

QuestTickResult::QuestTickResult(QuestModelAction questChanges) noexcept :
        questChanges(questChanges) { }

QuestTickResult::QuestTickResult(ID questId, std::vector<WorldModelAction> worldChanges) noexcept :
        worldChanges(worldChanges), questChanges(QuestModelAction(QuestActionType::KEEP, questId)) { }

vector<WorldModelAction> QuestTickResult::GetWorldChanges() const noexcept {
    return worldChanges;
}

QuestModelAction QuestTickResult::GetQuestChange() const noexcept {
    return questChanges;
}

std::string Quest::GetStory() const {
    return story;
}
