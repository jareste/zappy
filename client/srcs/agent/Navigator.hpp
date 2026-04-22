#pragma once

#include "../protocol/Message.hpp"

#include <utility>
#include <vector>

enum class NavCmd { Forward, TurnLeft, TurnRight };

namespace Navigator {
    std::pair<int, int> localToWorldDelta(Orientation facing, int localX, int localY);
    std::vector<NavCmd> turnToFace(Orientation current, Orientation target);
    std::vector<NavCmd> planPath(Orientation facing, int localX, int localY);
    std::vector<NavCmd> planApproachDirection(int broadcastDirection, Orientation currentFacing);
    std::vector<NavCmd> explorationStep(int& stepCount);
}