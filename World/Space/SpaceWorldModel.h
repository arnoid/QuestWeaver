//
// Created by michael on 06.10.15.
//

#pragma once

#include "../WorldModel.h"
#include "SpaceLocation.h"
#include "../../Core/WeaverUtils.h"

namespace weave {
    struct ModelParameters {
        int minLocation = -1000;
        int maxLocation = 1000;
    };

    class SpaceWorldModel : public WorldModel {
    public:
        explicit SpaceWorldModel(std::shared_ptr<RandomStream> randomStream);

        ModelParameters GetParameters();

        void SetParameters(ModelParameters parameters);

        // entity creation methods

        std::shared_ptr<SpaceLocation> CreateLocation() const;

    private:
        std::shared_ptr<RandomStream> rs;
        ModelParameters param;
    };
}
