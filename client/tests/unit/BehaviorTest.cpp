#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>

#include "../srcs/agent/Behavior.hpp"

using namespace testing;

class FakeSender : public Sender {
    public:
        std::string lastCmd;
        std::function<void(const ServerMessage&)> lastCallback;

        FakeSender(WebsocketClient& ws) : Sender(ws) {}

        Result sendVoir() override       { lastCmd = "voir";       return Result::success(); }
        Result sendInventaire() override { lastCmd = "inventaire"; return Result::success(); }
        Result sendGauche() override     { lastCmd = "gauche";     return Result::success(); }
        Result sendDroite() override     { lastCmd = "droite";     return Result::success(); }
        Result sendAvance() override     { lastCmd = "avance";     return Result::success(); }
        Result sendPrend(const std::string& resource) override {
            lastCmd = "prend " + resource;
            return Result::success();
        }

        void expect(const std::string&,
                    std::function<void(const ServerMessage&)> cb) override {
            lastCallback = cb;
        }

        void fireCallback(const ServerMessage& msg) {
            if (lastCallback) lastCallback(msg);
        }
};

static ServerMessage makeOkResponse(const std::string& cmd) {
    ServerMessage msg;
    msg.type   = MsgType::Response;
    msg.cmd    = cmd;
    msg.status = "ok";
    return msg;
}

static ServerMessage makeVoirResponse(std::vector<VisionTile> tiles) {
    ServerMessage msg = makeOkResponse("voir");
    msg.vision = tiles;
    return msg;
}

static ServerMessage makeInventaireResponse(Inventory inv) {
    ServerMessage msg = makeOkResponse("inventaire");
    msg.inventory = inv;
    return msg;
}

class MockWebsocketClient : public WebsocketClient {
    public:
        MOCK_METHOD(IoResult, sendText, (const std::string& text), (override));
};

class BehaviorTest : public ::testing::Test {
    protected:
        MockWebsocketClient ws;   // reuse the one from SenderTest
        FakeSender          sender{ws};
        WorldState          state;
        std::string         name = "team1";
        Behavior            behavior{sender, state, name};

        void SetUp() override {
            // vision and inventory start stale by default
        }

        // builds a minimal vision: one tile at distance 0 with given items
        VisionTile makeTile(int localX, int localY, std::vector<std::string> items) {
            VisionTile t;
            t.distance   = localY;
            t.localX     = localX;
            t.localY     = localY;
            t.playerCount = 0;
            t.items      = items;
            return t;
        }

        // gives behavior a fresh vision so it skips the stale check
        void giveFreshVision(std::vector<VisionTile> tiles) {
            state.vision = tiles;
            behavior.setVisionStale();

            // tick to send voir
            behavior.tick(0);
            ASSERT_EQ(sender.lastCmd, "voir");

            // fire voir callback
            sender.fireCallback(makeVoirResponse(tiles));
        }

        void giveFreshInventory(Inventory inv) {
            state.player.inventory = inv;
            behavior.setInventoryStale();

            behavior.tick(0);
            ASSERT_EQ(sender.lastCmd, "inventaire");

            sender.fireCallback(makeInventaireResponse(inv));
        }

        void levelUpPlayer() {
			if (state.player.level < 8) state.player.level++;

			return;
		};
};

TEST_F(BehaviorTest, StaleVisionSendsVoir) {
    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "voir");
    EXPECT_TRUE(behavior.hasCommandInFlight());
}

TEST_F(BehaviorTest, VoirCallbackClearsInflightAndUpdatesVision) {
    behavior.tick(0);
    ASSERT_EQ(sender.lastCmd, "voir");

    std::vector<VisionTile> tiles = { makeTile(0, 0, {}) };
    sender.fireCallback(makeVoirResponse(tiles));

    EXPECT_FALSE(behavior.hasCommandInFlight());
    EXPECT_FALSE(behavior.isVisionStale());
    EXPECT_EQ(state.vision.size(), 1u);
}

TEST_F(BehaviorTest, CommandInFlightBlocksTick) {
    behavior.tick(0); // sends voir, sets in-flight
    ASSERT_TRUE(behavior.hasCommandInFlight());

    sender.lastCmd = ""; // clear so a second send can be detected
    behavior.tick(0);   // should do nothing
    EXPECT_EQ(sender.lastCmd, "");
}

TEST_F(BehaviorTest, StaleInventorySendsInventaireAfterVision) {
    // satisfy vision first
    giveFreshVision({ makeTile(0, 0, {}) });

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "inventaire");
}

TEST_F(BehaviorTest, InventaireCallbackUpdatesInventory) {
    giveFreshVision({ makeTile(0, 0, {}) });

    behavior.tick(0);
    ASSERT_EQ(sender.lastCmd, "inventaire");

    Inventory inv;
    inv.nourriture = 5;
    sender.fireCallback(makeInventaireResponse(inv));

    EXPECT_FALSE(behavior.isInventoryStale());
    EXPECT_EQ(state.player.inventory.nourriture, 5);
}

TEST_F(BehaviorTest, FoodOnCurrentTileSendsPrend) {
    giveFreshVision({ makeTile(0, 0, {"nourriture"}) });
    giveFreshInventory({});

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "prend nourriture");
}

TEST_F(BehaviorTest, PrendOkIncrementsInventory) {
    giveFreshVision({ makeTile(0, 0, {"nourriture"}) });
    giveFreshInventory({});

    behavior.tick(0);
    ASSERT_EQ(sender.lastCmd, "prend nourriture");

    sender.fireCallback(makeOkResponse("prend"));
    EXPECT_EQ(state.player.inventory.nourriture, 1);
    EXPECT_TRUE(behavior.isInventoryStale());
}

TEST_F(BehaviorTest, FoodToLeftSendsGauche) {
    giveFreshVision({
        makeTile(0, 0, {}),
        makeTile(-1, 1, {"nourriture"})
    });
    giveFreshInventory({});

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "gauche");
}

TEST_F(BehaviorTest, FoodToRightSendsDroite) {
    giveFreshVision({
        makeTile(0, 0, {}),
        makeTile(1, 1, {"nourriture"})
    });
    giveFreshInventory({});

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "droite");
}

TEST_F(BehaviorTest, FoodAheadSendsAvance) {
    giveFreshVision({
        makeTile(0, 0, {}),
        makeTile(0, 1, {"nourriture"})
    });
    giveFreshInventory({});

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "avance");
}

TEST_F(BehaviorTest, NoFoodVisibleSendsDroiteForExploration) {
    giveFreshVision({ makeTile(0, 0, {}) });
    giveFreshInventory({});

    behavior.tick(0);
    EXPECT_EQ(sender.lastCmd, "droite");
}

TEST_F(BehaviorTest, MovementCallbackSetsVisionStale) {
    giveFreshVision({ makeTile(0, 0, {}) });
    giveFreshInventory({});

    behavior.tick(0); // sends droite (exploration)
    ASSERT_EQ(sender.lastCmd, "droite");
    ASSERT_FALSE(behavior.isVisionStale());

    sender.fireCallback(makeOkResponse("droite"));
    EXPECT_TRUE(behavior.isVisionStale());
}

TEST_F(BehaviorTest, ComputeMissingStonesReturnsCorrectState) {
    std::vector<VisionTile> tiles = { makeTile(0, 0, {}) };
    giveFreshVision(tiles);
    behavior.computeMissingStones();

    ASSERT_EQ(behavior.getStonesNeeded().size(), 1u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "linemate");

    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 3u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "deraumere");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "linemate");

    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 3u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "phiras");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "linemate");

    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 4u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "phiras");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "deraumere");
    EXPECT_EQ(behavior.getStonesNeeded()[3], "linemate");

    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 4u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "mendiane");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "deraumere");
    EXPECT_EQ(behavior.getStonesNeeded()[3], "linemate");

    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 4u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "phiras");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "deraumere");
    EXPECT_EQ(behavior.getStonesNeeded()[3], "linemate");
    
    levelUpPlayer();
    behavior.computeMissingStones();
    ASSERT_EQ(behavior.getStonesNeeded().size(), 6u);
    EXPECT_EQ(behavior.getStonesNeeded()[0], "thystame");
    EXPECT_EQ(behavior.getStonesNeeded()[1], "phiras");
    EXPECT_EQ(behavior.getStonesNeeded()[2], "mendiane");
    EXPECT_EQ(behavior.getStonesNeeded()[3], "sibur");
    EXPECT_EQ(behavior.getStonesNeeded()[4], "deraumere");
    EXPECT_EQ(behavior.getStonesNeeded()[5], "linemate");
}