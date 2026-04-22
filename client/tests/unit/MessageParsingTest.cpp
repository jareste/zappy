#include <gtest/gtest.h>
#include "../../srcs/protocol/MessageParser.hpp"
#include "../../incs/DataStructs.hpp"
#include "../../srcs/helpers/Logger.hpp"

TEST(MessageParserTest, WelcomeResponse) {
	const std::string raw = 
		R"({"type":"welcome","remaining_clients":3,"map_size":{"x":10,"y":10}})";
	
	ServerMessage msg = MessageParser::parse(raw);

	EXPECT_EQ(msg.type, MsgType::Welcome);
	EXPECT_EQ(msg.remainingSlots, 3);
	EXPECT_EQ(msg.mapWidth, 10);
	EXPECT_EQ(msg.mapHeight, 10);
}

TEST(MessageParserTest, VoirOneTile) {
	const std::string raw =
		R"({"type":"response","cmd":"voir","vision":[["player","nourriture", "phiras", "sibur"]]})";

	ServerMessage msg = MessageParser::parse(raw);

	ASSERT_EQ(msg.type, MsgType::Response);
	ASSERT_EQ(msg.cmd, "voir");
	ASSERT_TRUE(msg.vision.has_value());

	const auto& tiles = msg.vision.value();
	ASSERT_EQ(static_cast<int>(tiles.size()), 1);

	// tile 0: player's own tile (d=0, localX=0, localY=0)
	EXPECT_EQ(tiles[0].distance, 0);
	EXPECT_EQ(tiles[0].localX,   0);
	EXPECT_EQ(tiles[0].localY,   0);
	EXPECT_EQ(tiles[0].playerCount, 1);
	EXPECT_TRUE(tiles[0].hasItem("phiras"));
	EXPECT_TRUE(tiles[0].hasItem("sibur"));
	EXPECT_TRUE(tiles[0].hasItem("nourriture"));
}

TEST(MessageParserTest, VoirFourTiles) {
	const std::string raw =
		R"({"type":"response","cmd":"voir","vision":[)"
		R"(["player"],)"           // d=0: own tile
		R"(["linemate"],)"         // d=1, i=0: left  (localX=-1)
		R"(["sibur","phiras"],)"   // d=1, i=1: center (localX=0)
		R"([])"                    // d=1, i=2: right (localX=+1)
		R"(]})";

	ServerMessage msg = MessageParser::parse(raw);

	ASSERT_TRUE(msg.vision.has_value());
	const auto& tiles = msg.vision.value();
	ASSERT_EQ(static_cast<int>(tiles.size()), 4);

	// d=0
	EXPECT_EQ(tiles[0].distance, 0);
	EXPECT_EQ(tiles[0].localX,   0);

	// d=1, left
	EXPECT_EQ(tiles[1].distance, 1);
	EXPECT_EQ(tiles[1].localX,  -1);
	EXPECT_TRUE(tiles[1].hasItem("linemate"));

	// d=1, center
	EXPECT_EQ(tiles[2].distance, 1);
	EXPECT_EQ(tiles[2].localX,   0);
	EXPECT_TRUE(tiles[2].hasItem("sibur"));
	EXPECT_TRUE(tiles[2].hasItem("phiras"));

	// d=1, right
	EXPECT_EQ(tiles[3].distance, 1);
	EXPECT_EQ(tiles[3].localX,  +1);
	EXPECT_EQ(static_cast<int>(tiles[3].items.size()), 0);
}

TEST(MessageParserTest, InventaireResponse) {
	const std::string raw =
		R"({"type":"response","cmd":"inventaire","status":"ok",)"
		R"("inventaire":{"nourriture":5,"linemate":1,"deraumere":0,)"
		R"("sibur":2,"mendiane":0,"phiras":3,"thystame":0}})";

	ServerMessage msg = MessageParser::parse(raw);

	ASSERT_EQ(msg.type, MsgType::Response);
	ASSERT_EQ(msg.cmd, "inventaire");
	ASSERT_TRUE(msg.isOk());
	ASSERT_TRUE(msg.inventory.has_value());

	const auto& inv = msg.inventory.value();
	EXPECT_EQ(inv.nourriture, 5);
	EXPECT_EQ(inv.linemate,   1);
	EXPECT_EQ(inv.deraumere,  0);
	EXPECT_EQ(inv.sibur,      2);
	EXPECT_EQ(inv.mendiane,   0);
	EXPECT_EQ(inv.phiras,     3);
	EXPECT_EQ(inv.thystame,   0);
}

TEST(MessageParserTest, BroadcastWithDirection) {
	const std::string raw =
		R"({"type":"message","arg":"regroup now","status":"3"})";

	ServerMessage msg = MessageParser::parse(raw);

	ASSERT_EQ(msg.type, MsgType::Broadcast);
	ASSERT_TRUE(msg.messageText.has_value());
	ASSERT_TRUE(msg.broadcastDirection.has_value());

	EXPECT_EQ(msg.messageText.value(),       "regroup now");
	EXPECT_EQ(msg.broadcastDirection.value(), 3);
}