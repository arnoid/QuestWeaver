//
// Created by michael on 29.01.16.
//

#pragma once

#include <Core/WeaverTypes.h>
#include <Story/StoryLine.h>
#include <Core/Graph/WeaverGraph.h>
#include <World/WorldEntity.h>

namespace weave {
    struct RawStoryLine {
        std::string prePart;
        std::string postPart;
        std::set<std::string> nuggets;
    };

    class StoryTemplate {
    public:
        explicit StoryTemplate(std::vector<RawStoryLine> rawStoryLines);

        virtual ~StoryTemplate() = default;

        //TODO return a set
        virtual std::vector<std::string> GetRequiredEntities() const = 0;

        virtual std::vector<StoryLine> CreateStory(
                std::map<std::string, std::vector<std::shared_ptr<WorldEntity>>> requiredEntities,
                const WeaverGraph &graph) const = 0;

    protected:
        std::vector<RawStoryLine> rawLines;
    };
}
