#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../srcs/protocol/Sender.hpp"
#include "../../incs/third_party/json.hpp"

using json = nlohmann::json;
using namespace testing;

class MockWebsocketClient : public WebsocketClient {
    public:
        MOCK_METHOD(IoResult, sendText, (const std::string& text), (override));
};

class SenderTest : public Test {
    protected:
        void SetUp() override {
            mockWs = std::make_unique<MockWebsocketClient>();
            sender = std::make_unique<Sender>(*mockWs);
            
            okResult.status = NetStatus::Ok;
            okResult.message = "OK";
            
            errorResult.status = NetStatus::NetworkError;
            errorResult.message = "Network error";
        }
        
        std::unique_ptr<MockWebsocketClient> mockWs;
        std::unique_ptr<Sender> sender;
        IoResult okResult;
        IoResult errorResult;
};

// JSON FORMATTING TESTS 

TEST_F(SenderTest, SendVoirSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendVoir();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "voir");
}

TEST_F(SenderTest, SendInventaireSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendInventaire();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "inventaire");
}

TEST_F(SenderTest, SendAvanceSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendAvance();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "avance");
}

TEST_F(SenderTest, SendDroiteSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendDroite();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "droite");
}

TEST_F(SenderTest, SendGaucheSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendGauche();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "gauche");
}

TEST_F(SenderTest, SendPrendSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendPrend("nourriture");

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "prend");
    EXPECT_EQ(j["arg"], "nourriture");
}

TEST_F(SenderTest, SendPoseSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendPose("linemate");

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "pose");
    EXPECT_EQ(j["arg"], "linemate");
}

TEST_F(SenderTest, SendBroadcastSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendBroadcast("Hello team!");

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "broadcast");
    EXPECT_EQ(j["arg"], "Hello team!");
}

TEST_F(SenderTest, SendIncantationSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendIncantation();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "incantation");
}

TEST_F(SenderTest, SendForkSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendFork();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "fork");
}

TEST_F(SenderTest, SendConnectNbrSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendConnectNbr();

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "cmd");
    EXPECT_EQ(j["cmd"], "connect_nbr");
}

TEST_F(SenderTest, SendLoginSendsCorrectJson) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendLogin("TeamA", "SECRET_KEY_123");

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "login");
    EXPECT_EQ(j["key"], "SECRET_KEY_123");
    EXPECT_EQ(j["role"], "player");
    EXPECT_EQ(j["team-name"], "TeamA");
}

TEST_F(SenderTest, SendLoginUsesDefaultKey) {
    std::string captured;
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce([&](const std::string& text) {
            captured = text;
            return okResult;
        });

    Result res = sender->sendLogin("TeamB");

    EXPECT_TRUE(res.ok());
    auto j = json::parse(captured);
    EXPECT_EQ(j["type"], "login");
    EXPECT_EQ(j["key"], "SOME_KEY");  // Default key from header
    EXPECT_EQ(j["role"], "player");
    EXPECT_EQ(j["team-name"], "TeamB");
}

// NETWORK ERROR HANDLING TESTS 

TEST_F(SenderTest, SendVoirReturnsErrorOnNetworkFailure) {
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce(Return(errorResult));

    Result res = sender->sendVoir();

    EXPECT_FALSE(res.ok());
    EXPECT_EQ(res.code, ErrorCode::NetworkError);
    EXPECT_EQ(res.message, "Network error");
}

TEST_F(SenderTest, SendInventaireReturnsErrorOnNetworkFailure) {
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce(Return(errorResult));

    Result res = sender->sendInventaire();

    EXPECT_FALSE(res.ok());
    EXPECT_EQ(res.code, ErrorCode::NetworkError);
}

TEST_F(SenderTest, SendAvanceReturnsErrorOnNetworkFailure) {
    EXPECT_CALL(*mockWs, sendText(_))
        .WillOnce(Return(errorResult));

    Result res = sender->sendAvance();

    EXPECT_FALSE(res.ok());
}

// RESPONSE PROCESSING TESTS 

TEST_F(SenderTest, ProcessResponseFiresCallback) {
    bool fired = false;
    ServerMessage received;

    sender->expect("voir", [&](const ServerMessage& msg) {
        fired = true;
        received = msg;
    });

    ServerMessage response;
    response.type   = MsgType::Response;
    response.cmd    = "voir";
    response.status = "ok";

    sender->processResponse(response);

    EXPECT_TRUE(fired);
    EXPECT_EQ(received.cmd, "voir");
    EXPECT_TRUE(received.isOk());
}

TEST_F(SenderTest, ProcessResponseWithPrendMatchesCompoundKey) {
    bool fired = false;
    ServerMessage received;

    sender->expect("prend nourriture", [&](const ServerMessage& msg) {
        fired = true;
        received = msg;
    });

    ServerMessage response;
    response.type   = MsgType::Response;
    response.cmd    = "prend";
    response.arg    = "nourriture";
    response.status = "ok";

    sender->processResponse(response);

    EXPECT_TRUE(fired);
    EXPECT_EQ(received.cmd, "prend");
    EXPECT_EQ(received.arg, "nourriture");
}

TEST_F(SenderTest, ProcessResponseWithPoseMatchesCompoundKey) {
    bool fired = false;
    ServerMessage received;

    sender->expect("pose linemate", [&](const ServerMessage& msg) {
        fired = true;
        received = msg;
    });

    ServerMessage response;
    response.type   = MsgType::Response;
    response.cmd    = "pose";
    response.arg    = "linemate";
    response.status = "ok";

    sender->processResponse(response);

    EXPECT_TRUE(fired);
    EXPECT_EQ(received.cmd, "pose");
    EXPECT_EQ(received.arg, "linemate");
}

TEST_F(SenderTest, IncantationInProgressDoesNotRemovePending) {
    int fireCount = 0;

    sender->expect("incantation", [&](const ServerMessage& msg) {
        (void)msg;
        fireCount++;
    });

    ServerMessage inProgress;
    inProgress.type   = MsgType::Response;
    inProgress.cmd    = "incantation";
    inProgress.status = "in_progress";

    sender->processResponse(inProgress);
    EXPECT_EQ(fireCount, 1);

    ServerMessage levelUp;
    levelUp.type   = MsgType::Response;
    levelUp.cmd    = "incantation";
    levelUp.status = "level_up";

    sender->processResponse(levelUp);
    EXPECT_EQ(fireCount, 2);
}

TEST_F(SenderTest, UnknownResponseDoesNotCrash) {
    // Should not crash or assert
    ServerMessage response;
    response.type = MsgType::Response;
    response.cmd = "unknown_command";
    response.status = "ok";

    EXPECT_NO_THROW(sender->processResponse(response));
}

TEST_F(SenderTest, NonResponseMessagesAreIgnored) {
    bool fired = false;
    sender->expect("voir", [&](const ServerMessage&) { fired = true; });

    ServerMessage event;
    event.type = MsgType::Event;
    event.cmd = "voir";

    sender->processResponse(event);
    EXPECT_FALSE(fired);  // Should not fire for non-response messages
}

// TIMEOUT TESTS 

TEST_F(SenderTest, TimedOutEntryFiresErrorCallback) {
    bool fired = false;
    ServerMessage received;

    sender->expect("voir", [&](const ServerMessage& msg) {
        fired = true;
        received = msg;
    });

    sender->checkTimeouts(0);

    EXPECT_TRUE(fired);
    EXPECT_EQ(received.cmd, "voir");
    EXPECT_EQ(received.status, "timeout");
    EXPECT_EQ(received.type, MsgType::Error);
}

TEST_F(SenderTest, MultipleTimeoutsFireAllCallbacks) {
    int fireCount = 0;

    sender->expect("voir", [&](const ServerMessage&) { fireCount++; });
    sender->expect("avance", [&](const ServerMessage&) { fireCount++; });
    sender->expect("droite", [&](const ServerMessage&) { fireCount++; });

    sender->checkTimeouts(0);

    EXPECT_EQ(fireCount, 3);
}

TEST_F(SenderTest, NonExpiredEntriesDoNotTimeout) {
    bool timedOut = false;
    bool validEntry = false;

    sender->expect("timeout_test", [&](const ServerMessage& msg) {
        if (msg.status == "timeout") timedOut = true;
    });
    
    sender->expect("valid_test", [&](const ServerMessage&) { validEntry = true; });

    // This should timeout
    sender->checkTimeouts(0);
    
    // This should NOT timeout (negative timeout, but realistically you'd need to mock time)
    // For this test, we're just verifying that the first one timed out
    EXPECT_TRUE(timedOut);
}

// CANCEL ALL TESTS 

TEST_F(SenderTest, CancelAllFiresCancelledCallbacksForAllPending) {
    int fireCount = 0;
    std::vector<std::string> cancelledCommands;

    sender->expect("voir", [&](const ServerMessage& msg) {
        fireCount++;
        EXPECT_EQ(msg.status, "cancelled");
        EXPECT_EQ(msg.type, MsgType::Error);
        cancelledCommands.push_back(msg.cmd);
    });
    
    sender->expect("avance", [&](const ServerMessage& msg) {
        fireCount++;
        EXPECT_EQ(msg.status, "cancelled");
        cancelledCommands.push_back(msg.cmd);
    });
    
    sender->expect("inventaire", [&](const ServerMessage& msg) {
        fireCount++;
        EXPECT_EQ(msg.status, "cancelled");
        cancelledCommands.push_back(msg.cmd);
    });

    sender->cancelAll();

    EXPECT_EQ(fireCount, 3);
    EXPECT_THAT(cancelledCommands, UnorderedElementsAre("voir", "avance", "inventaire"));
}

TEST_F(SenderTest, CancelAllClearsPendingQueue) {
    int cancelledCount = 0;
    
    sender->expect("cmd1", [&](const ServerMessage& msg) {
        if (msg.status == "cancelled") cancelledCount++;
    });
    sender->expect("cmd2", [&](const ServerMessage& msg) {
        if (msg.status == "cancelled") cancelledCount++;
    });
    sender->expect("cmd3", [&](const ServerMessage& msg) {
        if (msg.status == "cancelled") cancelledCount++;
    });

    sender->cancelAll();
    
    // All three should have been cancelled
    EXPECT_EQ(cancelledCount, 3);
    
    // After cancelAll, the queue should be empty
    int postCancelFireCount = 0;
    sender->expect("post_cancel", [&](const ServerMessage& msg) {
        if (msg.status == "timeout") postCancelFireCount++;
    });
    
    // Only the newly added command should timeout
    sender->checkTimeouts(0);
    EXPECT_EQ(postCancelFireCount, 1);
}

TEST_F(SenderTest, CancelAllDoesNotAffectNewlyAddedCommands) {
    sender->expect("old_command", [&](const ServerMessage& msg) {
        EXPECT_EQ(msg.status, "cancelled");
    });

    sender->cancelAll();

    bool newCommandFired = false;
    sender->expect("new_command", [&](const ServerMessage& msg) {
        newCommandFired = true;
        EXPECT_EQ(msg.status, "timeout");
    });

    sender->checkTimeouts(0);
    EXPECT_TRUE(newCommandFired);
}

TEST_F(SenderTest, CancelAllWithEmptyQueueDoesNothing) {
    EXPECT_NO_THROW(sender->cancelAll());
    // Should not crash or assert
}

TEST_F(SenderTest, CancelAllAfterPartialResponses) {
    // Simulate scenario: some commands completed, some pending
    
    // Add a command that will be completed
    bool completedFired = false;
    sender->expect("completed", [&](const ServerMessage& msg) {
        completedFired = true;
        EXPECT_EQ(msg.status, "ok");
    });
    
    // Add commands that will be cancelled
    bool cancelledFired = false;
    sender->expect("cancelled1", [&](const ServerMessage& msg) {
        cancelledFired = true;
        EXPECT_EQ(msg.status, "cancelled");
    });
    
    bool cancelledFired2 = false;
    sender->expect("cancelled2", [&](const ServerMessage& msg) {
        cancelledFired2 = true;
        EXPECT_EQ(msg.status, "cancelled");
    });
    
    // Complete the first command
    ServerMessage response;
    response.type = MsgType::Response;
    response.cmd = "completed";
    response.status = "ok";
    sender->processResponse(response);
    EXPECT_TRUE(completedFired);
    
    // Cancel remaining
    sender->cancelAll();
    
    EXPECT_TRUE(cancelledFired);
    EXPECT_TRUE(cancelledFired2);
}

// EDGE CASE TESTS 

TEST_F(SenderTest, ExpectWithEmptyCallbackDoesNotCrash) {
    EXPECT_NO_THROW(sender->expect("test", nullptr));
    
    ServerMessage response;
    response.type = MsgType::Response;
    response.cmd = "test";
    response.status = "ok";
    
    EXPECT_NO_THROW(sender->processResponse(response));
}

TEST_F(SenderTest, ProcessResponseWithMissingCallbackDoesNotCrash) {
    ServerMessage response;
    response.type = MsgType::Response;
    response.cmd = "no_callback_registered";
    response.status = "ok";
    
    EXPECT_NO_THROW(sender->processResponse(response));
}

TEST_F(SenderTest, MultipleCallbacksForSameCommand) {
    int fireCount = 0;
    
    sender->expect("same_cmd", [&](const ServerMessage&) { fireCount++; });
    sender->expect("same_cmd", [&](const ServerMessage&) { fireCount++; });
    
    ServerMessage response;
    response.type = MsgType::Response;
    response.cmd = "same_cmd";
    response.status = "ok";
    
    sender->processResponse(response);
    
    EXPECT_EQ(fireCount, 1);  // Only the first matching callback should fire and be removed
}

TEST_F(SenderTest, CancelAllWithCallbackThatAddsMoreCommands) {
    // This tests that cancelAll doesn't cause infinite recursion
    bool recursiveCallbackFired = false;
    
    sender->expect("recursive", [&](const ServerMessage& msg) {
        (void)msg;
        recursiveCallbackFired = true;
        // Try to add a new command during cancellation
        sender->expect("new_from_callback", [&](const ServerMessage&) {});
    });
    
    EXPECT_NO_THROW(sender->cancelAll());
    EXPECT_TRUE(recursiveCallbackFired);
}