#include <gtest/gtest.h>

#include "../srcs/agent/Navigator.hpp"
#include "../srcs/protocol/Message.hpp"

#include <utility>
#include <vector>
#include <iostream>

TEST(Navigator, LocalToWorldDeltaConversionTest) {
    std::pair<int, int> orientation = Navigator::localToWorldDelta(
        Orientation::N, 1, 2);
    EXPECT_EQ(orientation.first, 1);
    EXPECT_EQ(orientation.second, -2);

    orientation = Navigator::localToWorldDelta(Orientation::E, 1, 2);
    EXPECT_EQ(orientation.first, 2);
    EXPECT_EQ(orientation.second, 1);

    orientation = Navigator::localToWorldDelta(Orientation::S, 1, 2);
    EXPECT_EQ(orientation.first, -1);
    EXPECT_EQ(orientation.second, 2);

    orientation = Navigator::localToWorldDelta(Orientation::W, 1, 2);
    EXPECT_EQ(orientation.first, -2);
    EXPECT_EQ(orientation.second, -1);
}

TEST(Navigator, PlanPathReturnsCorrectSequences) {
    std::vector<NavCmd> path = Navigator::planPath(
        Orientation::N, 0, 2);
    EXPECT_EQ(static_cast<int>(path.size()), 2);
    EXPECT_EQ(path[0], NavCmd::Forward);
    EXPECT_EQ(path[1], NavCmd::Forward);
    path.clear();

    path = Navigator::planPath(Orientation::N, 1, 0);
    EXPECT_EQ(static_cast<int>(path.size()), 3);
    EXPECT_EQ(path[0], NavCmd::TurnRight);
    EXPECT_EQ(path[1], NavCmd::Forward);
    EXPECT_EQ(path[2], NavCmd::TurnLeft);
    path.clear();

    path = Navigator::planPath(Orientation::N, -1, 2);
    EXPECT_EQ(static_cast<int>(path.size()), 5);
    EXPECT_EQ(path[0], NavCmd::TurnLeft);
    EXPECT_EQ(path[1], NavCmd::Forward);
    EXPECT_EQ(path[2], NavCmd::TurnRight);
    EXPECT_EQ(path[3], NavCmd::Forward);
    EXPECT_EQ(path[4], NavCmd::Forward);
    path.clear();

    path = Navigator::planPath(Orientation::E, 1, 1);
    EXPECT_EQ(static_cast<int>(path.size()), 4);
    EXPECT_EQ(path[0], NavCmd::TurnRight);
    EXPECT_EQ(path[1], NavCmd::Forward);
    EXPECT_EQ(path[2], NavCmd::TurnLeft);
    EXPECT_EQ(path[3], NavCmd::Forward);
    path.clear();

    path = Navigator::planPath(Orientation::E, 3, 7);
    EXPECT_EQ(static_cast<int>(path.size()), 12);
    EXPECT_EQ(path[0], NavCmd::TurnRight);
    EXPECT_EQ(path[1], NavCmd::Forward);
    EXPECT_EQ(path[2], NavCmd::Forward);
    EXPECT_EQ(path[3], NavCmd::Forward);
    EXPECT_EQ(path[4], NavCmd::TurnLeft);
    EXPECT_EQ(path[5], NavCmd::Forward);
    EXPECT_EQ(path[6], NavCmd::Forward);
    EXPECT_EQ(path[7], NavCmd::Forward);
    EXPECT_EQ(path[8], NavCmd::Forward);
    EXPECT_EQ(path[9], NavCmd::Forward);
    EXPECT_EQ(path[10], NavCmd::Forward);
    EXPECT_EQ(path[11], NavCmd::Forward);
}

TEST(Navigator, ExplorationTurnInjection) {
    int step = 0;

    auto cmds = Navigator::explorationStep(step);
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0], NavCmd::TurnRight);
    EXPECT_EQ(cmds[1], NavCmd::Forward);

    // Steps 2–6: plain Forward, no turn
    for (int i = 2; i <= 6; i++) {
        cmds = Navigator::explorationStep(step);
        ASSERT_EQ(cmds.size(), 1u) << "Expected no turn at step " << i;
        EXPECT_EQ(cmds[0], NavCmd::Forward);
    }

    // Step 7: % 7 == 0 → TurnRight + Forward
    cmds = Navigator::explorationStep(step);
    ASSERT_EQ(step, 7);
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0], NavCmd::TurnRight);
    EXPECT_EQ(cmds[1], NavCmd::Forward);

    // Steps 8–12: plain Forward
    for (int i = 8; i <= 12; i++) {
        cmds = Navigator::explorationStep(step);
        ASSERT_EQ(cmds.size(), 1u) << "Expected no turn at step " << i;
        EXPECT_EQ(cmds[0], NavCmd::Forward);
    }

    // Step 13: % 13 == 0 → TurnLeft wins
    cmds = Navigator::explorationStep(step);
    ASSERT_EQ(step, 13);
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0], NavCmd::TurnLeft);
    EXPECT_EQ(cmds[1], NavCmd::Forward);

    // Step 14: % 7 == 0 → TurnRight + Forward
    cmds = Navigator::explorationStep(step);
    ASSERT_EQ(step, 14);
    ASSERT_EQ(cmds.size(), 2u);
    EXPECT_EQ(cmds[0], NavCmd::TurnRight);
    EXPECT_EQ(cmds[1], NavCmd::Forward);
}