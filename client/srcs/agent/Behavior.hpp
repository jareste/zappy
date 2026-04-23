#pragma once

#include "../protocol/Message.hpp"
#include "State.hpp"
#include "../protocol/Sender.hpp"
#include "Navigator.hpp"

#include <cstdint>
#include <deque>

enum class AIState {
	Idle,				// 0
	CollectFood,		// 1
	CollectStones,		// 2
	Incantating,		// 3
	ClaimingLeader,		// 4
	Leading,			// 5
	MovingToRally,		// 6
	Rallying,			// 7
	Forking,			// 8
	WaitingForHatch,	//9
};

struct LevelReq {
	int         players;
	Inventory   stones;
};

class Behavior {
	private:
		Sender&						_sender;
		WorldState&					_state;
		bool						_commandInFlight = false;
		bool						_staleVision = true;
		bool						_staleInventory = true;
		std::deque<NavCmd> 			_navPlan;
		std::string					_navTarget;
		int							_explorationStep = 0;

		std::string					_teamName;

		AIState						_aiState = AIState::CollectFood;
		bool						_easyMode = false;
		bool						_pendingLevelUp = false;

		bool						_isLeader = false;
		bool						_isMovingToRally = false;
		bool						_isRallying = false;
		int							_rallyLevel = 0;
		int							_broadcastDirection = 0;
		int							_peerConfirmedCount = 0;
		int							_rallyBroadcastCount = 0;
		int64_t						_lastRallyBroadcastMs = 0;
		int64_t						_leadingTimeoutMs = 0;
		int64_t						_movingToRallyTimeoutMs = 0;
		int64_t						_rallyingTimeoutMs = 0;

		bool						_claimSent = false;
		bool						_ignoreDone = false;
		bool						_hereSent = false;
		bool						_shouldStopMoving = false;
		bool						_waitingForBroadcast = false;

		bool						_readyForIncantation = false;
    	int64_t						_readyForIncantationTime = 0;

		int64_t						_lastMovingToRallyVisionMs = 0;
		int64_t						_lastInventoryRefreshMs    = 0;

		Orientation					_broadcastReceivedFacing = Orientation::N;

		std::vector<std::string>    _stonesNeeded;
		bool                        _incantationReady;
		bool                        _stonesPlaced;
		bool                        _stonesReady = false;

		int64_t						_claimJitterEndMs = 0;
		int64_t						_lastTickMs = 0;

		bool						_forkInProgress = false;
		bool						_forkSent = false;
		int64_t						_forkSentMs = 0;
		int64_t						_lastForkMs = 0;

		int64_t						_hatchPollIntervalMs = 2000;
		int64_t						_lastHatchPollMs = 0;
		int64_t						_hatchTimeoutMs = 0;

		int							_pendingEggCount = 0;

		bool						_connectNbrInFlight = false;

		static constexpr int 		FOOD_FORK      = 24;
		static constexpr int 		FOOD_RALLY     = 16;
		static constexpr int 		FOOD_SAFE      = 12;
		static constexpr int 		FOOD_CRITICAL  = 6;

		static constexpr int 		FORK_MIN_LEVEL			= 2;
		static constexpr int 		HATCH_DELAY_UNITS		= 600;
		static constexpr int 		UNITS_PER_MS			= 10;
		static constexpr int 		HATCH_DELAY_MS			= HATCH_DELAY_UNITS * UNITS_PER_MS;
		static constexpr int 		HATCH_POLL_STARTS_MS	= HATCH_DELAY_MS + 1000;
		static constexpr int 		HATCH_TIMEOUT_MS		= 30000;
		static constexpr int 		MAX_PENDING_EGGS		= 2;
		static constexpr int64_t	FORK_COOLDOWN_MS	= 60000;

		void executeNavCmd(NavCmd cmd);

		void disbandRally(bool wasLeader);

	public:
		Behavior(Sender& sender, WorldState& state, std::string& teamName);
		~Behavior() = default;

		void tick(int64_t nowMs);
		void tickCollectFood();
		void tickCollectStones();
		void tickIdle();
		void tickIncantating();
		void tickClaimingLeader();
		void tickLeading(int64_t nowMs);
		void tickMovingToRally(int64_t nowMs);
		void tickRallying(int64_t nowMs);
		void tickForking();
		void tickWaitingForHatch(int64_t nowMs);

		void refreshVision();
		void refreshInventory();

		void onBroadcast(const ServerMessage& msg);

		AIState getState() const { return _aiState; }

		bool hasCommandInFlight() const { return _commandInFlight; }
		bool isVisionStale()      const { return _staleVision; }
		bool isInventoryStale()   const { return _staleInventory; }

		std::vector<std::string>& getStonesNeeded() { return _stonesNeeded; }

		void setVisionStale()    { _staleVision = true; }

		void setAIState(AIState s) { _aiState = s; }
		void setInventoryStale() { _staleInventory = true; }
		void clearNavPlan()      { _navPlan.clear(); _navTarget.clear(); }

		void computeMissingStones();
		VisionTile getNearestTileWithNeededResource();
		void setPendingLevelUp(bool val) { _pendingLevelUp = val; }
		void setEasyMode(bool enabled) { _easyMode = enabled; }

		bool shouldFork() const;
		void spawnChildClient();
};
