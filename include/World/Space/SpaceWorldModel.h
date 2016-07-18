//
// Created by michael on 06.10.15.
//

#pragma once

#include "../WorldModel.h"
#include "../../Core/WeaverUtils.h"
#include "SpaceNameGenerator.h"
#include "SolarSystem.h"
#include <World/Space/SpaceLocation.h>

namespace weave {
    struct ModelParameters {
        int minLocation = 0;
        int maxLocation = 1000;
        int minPlanets = 1;
        int maxPlanets = 8;
        int planetDistanceBase = 3000;
        int planetDistanceAverage = 5000;
        int planetDistanceVariation = 2000;
        int deadCivRarity = 20;

        int startSystems = 15;
        int startSpaceStations = 5;
        int startDeadCivs = 2;
        int startFriends = 3;
        int startNeutrals = 25;
        int startEnemies = 5;

        template<class Archive>
        void serialize(Archive &ar) {
            ar(CEREAL_NVP(minLocation), CEREAL_NVP(maxLocation));
        }
    };

    class SpaceWorldModel : public WorldModel {
    public:
        SpaceWorldModel() = default;

        explicit SpaceWorldModel(std::shared_ptr<RandomStream> randomStream);

        SpaceWorldModel(std::shared_ptr<RandomStream> randomStream, ModelParameters modelParameters);

        ModelParameters GetParameters() const;

        const NameGenerator &GetNameGenerator() const override;

        /**
         * Creates a complete new world with a reasonable number of different entities.
         */
        std::vector<WorldModelAction> InitializeNewWorld() const;

        // entity creation methods

        WorldModelAction CreateLocation() const;

        WorldModelAction CreateAgent(NameType nameType = NameType::LIGHT_PERSON) const;

        WorldModelAction CreatePlanet(std::shared_ptr<SpaceLocation> location,
                                      NameType nameType = NameType::DARK_THING, int distanceToSun = 0) const;

        std::vector<WorldModelAction> CreateSolarSystem(NameType nameType = NameType::LIGHT_THING,
                                                        int planetCount = -1) const;

        WorldModelAction CreateDeadCivilization(NameType nameType = NameType::ALIEN) const;

        WorldModelAction CreateArtifact(NameType nameType = NameType::ALIEN) const;

        WorldModelAction CreateSpaceStation(std::shared_ptr<SolarSystem> homeSystem,
                                            NameType nameType = NameType::LIGHT_THING) const;

    private:
        ModelParameters param;
        SpaceNameGenerator nameGenerator;

        // serialization
        friend class cereal::access;

        template<class Archive>
        void serialize(Archive &ar) {
            ar(cereal::base_class<WorldModel>(this), CEREAL_NVP(param));
        }
    };
}

CEREAL_REGISTER_TYPE(weave::SpaceWorldModel);
