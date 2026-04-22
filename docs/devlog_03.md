# Zappy Client - Devlog - 3

## Table of Contents
1. [The Hardest Choices Require The Strongests Wills](#31---the-hardest-choices-require-the-strongests-wills)
2. [We Don't Walk Alone](#32-we-dont-walk-alone)
3. [As the Crow Flies](#33-as-the-crow-flies)
	- [I've Made Plans and I Know Exactly Where I'm Going](#331-ive-made-plans-and-i-know-exactly-where-im-going---planpath)
	- [I Have No Target and I Must Move](#332-i-have-no-target-and-i-must-move---explorationstep)
	- [Maybe It Was *Not* Just The Wind](#333-maybe-it-was-not-just-the-wind---planapproachdirection)
	- [Execute Order <insert_number_here_I_refuse_writing_66>](#334-execute-order-insert_number_here_i_refuse_to_write_66)
	- [I Love It When a Plan Comes Together, and I Must Learn To Love It When It Doesn't Too](#335-i-love-it-when-a-plan-comes-together-and-i-must-learn-to-love-it-when-it-doesnt-too)
4. [Rally-Ho!](#34-rally-ho)


<br>
<br>

# 3.1 - The Hardest Choices Require The Strongests Wills
After reaching or first milestone, we can be sure (or as sure as we can be) about having a working base that 1) acts as a checkpoint in case Charles The Bot The Third kicks the bucket 2) makes the road ahead a pure matter of extending. This means that our next milestone, which is to have a single little dude reach max level by itself in easy mode, as well as anything that comes later, is just going to ask us to work on adding layers of possibility and complexity to `Behavior` and `Navigator`. Tools and pipelines for stone gathering, rallying and forking, those things that are still pending between us and a full, non-easy winning game bot. I trust in Charles III, but we'll have to see if it believes in itself.

So, as I said, the first goal going forward is to make our little dude gather stones, Thanos-style, and our first small step should be to make our probed single little dude to reach level 2, as the 1→2 transition is the only one that doesn't require aditional participants beyond the S E L F. Doing so is going to require:
- New `CollectStones` and `Incantation` states added to the enum of the machine
- A table of stone and player requirements for each level transition (we'll only use the first one for now, but we'll need it for the next step so let's get it out of our way)
- Create transition patters between them
- Maintain a safe lock for emergency food levels (i.e., triangulate these two new states with the already existing `CollectFood`)
- Add navigation path searching targetting stones
- Add incantation triggers and managers

That sould do it. And with or new list in our hands, let's get into implementations. First, and after extending the `AIState` collection, we'll expand `Behavior`'s attributes to track its `AIState`, and three level-up related additions (`_stonesNeeded`, `_incantationSet`, `_stonesPlaced`) to track the progress of the ascension ritual. After this, our next important change is related to how `tick()` works in `Behavior`. Up until this point, our little dude has just been following a fixed pattern without regards to its state, but we have to transition into a proper state-machine like state-depending ticking. To do so, we'll refactor `tick()` to be an entry point to the ticking process, containing just a switch case on the current `AIState` in order to derive execution to specific ticking sub-functions, all while keeping the existing top-checks. Something like this:
```cpp
void Behavior::tick(int64_t nowMs) {
    if (hasCommandInFlight()) return;
    if (isVisionStale())      { refreshVision(); return; }
    if (isInventoryStale())   { refreshInventory(); return; }

    switch (_aiState) {
        case AIState::CollectFood:      tickCollectFood(); break;
        case AIState::CollectStones:    tickCollectStones(); break;
        case AIState::Incantating:      tickIncantating(); break;
        case AIState::Idle:             break;
    }
}
```

For clarity sakes, we'll get the refreshing stuff to specific functions. Then, everything else that was inside `tick()`, which is basically the food-gathering behavior, will be taken to `tickCollectFood()`, as-is, just with the state transition check and execution added to the top of the function. A very simple one: **if the amount of food in inventory is higher than the safe threshold set up as `FOOD_SAFE`, `_aiState` will go from `CollectFood` to `CollectStones`. Easy stuff.

Now, for the stone collection behavior, housed in `tickCollectStones()`, we'll need to do the following:
- For safety, we'll first check if while searching for stones food reservers went below the critical threashold, and if so we'll have the AI transition its state back to `CollectFood`
	- We'll make this check against `FOOD_CRITICAL` and not `FOOD_SAFE` because if did so, we'll have a little dude ping-pong-ing between states, looping endlessly, lost forever.
- Compute missing stones
- If there are no missing stones, state will transition to `Incantating`
- Else we'll have our little dude find the nearest tile with a needed resource, prioritizing distance over anything else (i.e., the check must be done tile-based, not missing-stone-based)
- After finding it, we'll have the little dude plan a stone-targetting navigation

Writing this logic is not a big deal. At the end of the day, `tickCollectStones()` is one of those flow functions in which what's really important is the order of checks and operations. The AI needs to know what stones it needs to search for, and once it knows what it is looking for, the logic is practically the same as looking for `nourriture` items. So, really, the important stuff here is the implementation of a couple of helpers, `computeMissingStones()` to acquire stone targets, and `getNearestTileWithNeededResourcer()` to locate them.

`ComputeMissingStones()` is just a checkup against the pre-made table of requirements per level transition. A `LevelReq` struct placed in `Behaviour.hpp`, tracking the amount of players for an incantation ritual, and the specific amount of stones (as an `Inventory` object) suffices to set up a static function that creates the mentioned table. Then it's just a question of pure comparison and detection:
```cpp
static const LevelReq& levelReq(int level) {
	// recipe: index 0 = level 1->2, index 6 = level 7->8
	// order: players -> nourriture, linemate, deraumere, sibur, mendiane, phiras, thystame
	static const LevelReq table[7] = {
		{ 1, { 0, 1, 0, 0, 0, 0, 0 } }, // 1→2
        { 2, { 0, 1, 1, 1, 0, 0, 0 } }, // 2→3
        { 2, { 0, 2, 0, 1, 0, 2, 0 } }, // 3→4
        { 4, { 0, 1, 1, 2, 0, 1, 0 } }, // 4→5
        { 4, { 0, 1, 2, 1, 3, 0, 0 } }, // 5→6
        { 6, { 0, 1, 2, 3, 0, 1, 0 } }, // 6→7
        { 6, { 0, 2, 2, 2, 2, 2, 1 } }, // 7→8
	};

	if (level < 1 || level > 7)
		return table[0]; // never going to happen, but its a safe fallback
	return table[level - 1];
}
```
```cpp
void Behavior::computeMissingStones() {
	if (_state.vision.empty()) return;
	
	int currentLevel = _state.player.level;
	Inventory& inv = _state.player.inventory;
	
	_stonesNeeded.clear();
	
	// easy mode -> 1 lineamte is enough
	if (_easyMode) {
		if (inv.linemate < 1) {
			_stonesNeeded.push_back("linemate");
		}
		return;
	}
	
	// normal mode
	const LevelReq& requirements = levelReq(currentLevel);
	if (requirements.stones.linemate  > inv.linemate)  _stonesNeeded.push_back("linemate");
	if (requirements.stones.deraumere > inv.deraumere) _stonesNeeded.push_back("deraumere");
	if (requirements.stones.sibur     > inv.sibur)     _stonesNeeded.push_back("sibur");
	if (requirements.stones.mendiane  > inv.mendiane)  _stonesNeeded.push_back("mendiane");
	if (requirements.stones.phiras    > inv.phiras)    _stonesNeeded.push_back("phiras");
	if (requirements.stones.thystame  > inv.thystame)  _stonesNeeded.push_back("thystame");
}
```

This all that our little guys need to roam around and gather stones, wrapped around some state transitions that take them back to food gathering if they go below the emergency threshold, and go forward into `Incantating` state if the set of required stones are in their inventory. A couple of notes here, though:
- When I first wrote `tickCollectStones()`, I considered the stones in the current tile of the little guy as part of a total sum for the requirement comparison. This didn't work properly. There's risks to doing so because the client might pass, say, a linemate check because there's linemate in its current position, but after detecting that it needs to move to get some deraumere that check would fail, and maybe this happens after confirming that deraumere is in place, but not because it was picked but because the target, deraumere-containing tile was reached... And so on and so forth. I guess that there must be a way to consider the stones already existing in tiles to make the whole game-playing process more optimal, but it feels like it would be difficult to pinpoint it and not worth it. So, for now and most likely forever, the requirements for a level up transition will only be checked against the client's inventory.
- At this stage/milestone, we're just working with a single little guy, which can reach max level and win the game if the session is set up in easy mode (via the env variable `ZAPPY_EASY_ASCENSION=1`). Until we transition to the next milestone, concerning multi-client cooperation, special considerations regarding the "role" a client assumes when going into `Incantating` state are irrelevant, i.e. not yet implemented. Maybe I'll have to set up class attributes to track the existance and identity of a "leader", I don't know. We'll see

What's important is that with all of the above in place, `tickCollectStones()` can be written:
```cpp
void Behavior::tickCollectStones() {
    if (_state.player.food() < FOOD_CRITICAL) {
        _aiState = AIState::CollectFood;
        clearNavPlan();
        return;
    }

    computeMissingStones();

    if (_stonesNeeded.empty()) {
        _aiState = AIState::Incantating;
		_incantationReady = false;
        clearNavPlan();
        return;
    }

    // pick up needed stone if already standing on one
    for (const auto& stone : _stonesNeeded) {
        if (_state.countItemOnCurrentTile(stone)) {
            clearNavPlan();
            _commandInFlight = true;
            _sender.sendPrend(stone);
            _sender.expect("prend " + stone, [this](const ServerMessage& msg) {
                _commandInFlight = false;
                if (msg.isOk())
                    setInventoryStale();
                setVisionStale();
            });
            return;
        }
    }

    // navigate towards nearest needed resource
    clearNavPlan();
    auto tile = getNearestTileWithNeededResource();

    if (tile.localX == std::numeric_limits<int>::max()) {
        // nothing visible yet->explore
        std::vector<NavCmd> plan = Navigator::explorationStep(_explorationStep);
        _navPlan.assign(plan.begin(), plan.end());
        _navTarget.clear();
    } else {
        std::vector<NavCmd> plan = Navigator::planPath(
            _state.player.orientation, tile.localX, tile.localY);
        Logger::debug("Behavior: CollectStones: planned " + std::to_string(plan.size()) +
            " steps to " + _navTarget + " at (" +
            std::to_string(tile.localX) + "," + std::to_string(tile.localY) + ")");
        _navPlan.assign(plan.begin(), plan.end());
    }

    if (!_navPlan.empty()) {
        NavCmd next = _navPlan.front();
        _navPlan.pop_front();
        executeNavCmd(next);
    }
}
```

`tickIncantating()` is going to be a little bit trickier, though. But we can do it, remember, we have a system for tackling things: let's think about what this needs to do. Everything we need is in of these places: past logs, our head, our heart, God. So we just have to connect with the correct source of knowledge to build know that the function is going to need the following steps:
1. Be sure that it's working in a non-stale vision state, triggering a `refreshVision()` call before moving on
2. Place the required stones for the level transition in the floor (as stated by the rules of the game).
	- This is the most tricky part, really, but deep down is just a thorough cross-check between the transition requirements, the tile contents and the little dude's inventory, which has to halt incantation ticking progress until all stones are in place.
3. The third step is a safety check wrapping the previous step, a way of being 100% sure that the tile contains the necessar stones
4. Once the previous check passes, `_sender.sendIncantation()` can be fired, with some detailed handling of the callback sent to the `expect()` function, as incantation is a two step process: it get's requested and aknowledge first, it gets resolved second.
5. Whatever the result is once an incantation is fired, the exit transition from `Incantating` is set up towards `CollectStones`. Back there, food related concernes will be managed, and the client will keep on playing the game normally.

In code, all of this looks like the following, super cute function:
```cpp
void Behavior::tickIncantating() {
	// Step 1: Get a fresh vision first
	if (!_incantationReady) {
		setVisionStale();
		_incantationReady = true;
		return;
	}

	// Step 2: Place required stones (one per tick)
	if (!_stonesPlaced) {
		if (_easyMode) {
			// Easy mode: only need to place 1 linemate
			auto& tile = _state.vision[0];
			if (tile.countItem("linemate") < 1) {
				_commandInFlight = true;
				_sender.sendPose("linemate");
				_sender.expect("pose linemate", [this](const ServerMessage& msg) {
					_commandInFlight = false;
					if (msg.isOk()) {
						_state.player.inventory.linemate--;
						setInventoryStale();
					}
					setVisionStale();
				});
				return;
			}
		} else {
			// Normal mode: place all required stones
			auto requirements = levelReq(_state.player.level);
			auto& tile = _state.vision[0];

			#define TRY_POSE(stone_name) \
				if (requirements.stones.stone_name > 0 && \
					tile.countItem(#stone_name) < requirements.stones.stone_name) { \
					_commandInFlight = true; \
					_sender.sendPose(#stone_name); \
					_sender.expect("pose " #stone_name, [this](const ServerMessage& msg) { \
						_commandInFlight = false; \
						if (msg.isOk()) { \
							_state.player.inventory.stone_name--; \
							setInventoryStale(); \
						} \
						setVisionStale(); \
					}); \
					return; \
				}

			TRY_POSE(linemate)
			TRY_POSE(deraumere)
			TRY_POSE(sibur)
			TRY_POSE(mendiane)
			TRY_POSE(phiras)
			TRY_POSE(thystame)
			#undef TRY_POSE
		}

		_stonesPlaced = true;
		setVisionStale();
		return;
	}

	// Step 3: Verify stones - simplified for easy mode
	if (_staleVision) return;
	
	auto& tile = _state.vision[0];
	
	bool stonesOk;
	if (_easyMode) {
		stonesOk = tile.countItem("linemate") >= 1;
	} else {
		auto requirements = levelReq(_state.player.level);
		stonesOk = tile.countItem("linemate")  >= requirements.stones.linemate  &&
				tile.countItem("deraumere") >= requirements.stones.deraumere &&
				tile.countItem("sibur")     >= requirements.stones.sibur     &&
				tile.countItem("mendiane")  >= requirements.stones.mendiane  &&
				tile.countItem("phiras")    >= requirements.stones.phiras    &&
				tile.countItem("thystame")  >= requirements.stones.thystame;
	}

	if (!stonesOk) {
		Logger::warn("Behavior: stones missing on tile after placement, back to CollectStones");
		_stonesPlaced = false;
		_incantationReady = false;
		_aiState = AIState::CollectStones;
		return;
	}

	// Step 4: Send incantation
	_commandInFlight = true;
	_sender.sendIncantation();
	_sender.expect("incantation", [this](const ServerMessage& msg) {
		if (msg.isInProgress()) {
			return;
		}

		_commandInFlight = false;
		_stonesPlaced = false;
		_incantationReady = false;

		if (_pendingLevelUp) {
			_state.player.level++;
			Logger::info("Level up! Now level " + std::to_string(_state.player.level));
			_pendingLevelUp = false;
			_aiState = (_state.player.level >= 8) ? AIState::Idle : AIState::CollectStones;
		} else {
			Logger::warn("Incantation failed (ko or timeout), restarting stone collection");
			_aiState = AIState::CollectStones;
		}
	});
}
```
And now, assuming that everything is correctly coded, what we should expect when running a single client probe test with the running server in easy mode is a little dude thriving, surviving, ascending and winning. There are some edits that need to be done here and there, both in client and server, to correctly handle the mentioned safe mode, detect winning conditions and trigger the endgame, but those are just uninteresting work. Which I've allready done...

... And my little dude is working correctly!!! And I'm happy!!! And the current milestone is done!! And we're almost there!! Not really because what's left is kind of the hardes part but hey we're fine!!! We're alive!!!

<br>
<br>

# 3.2 We Don't Walk Alone
Our next milestone is to extend the stone gathering behavior so that **every level requirements are correctly gathered** in a non-easy context. We'll have to be careful to not break the 1→2 trasition (which little dudes can do by themselves), but we'll have to test level after level if the clients target the rocks the are supposed to. We'll do so while also injecting some resource type priorities (rarest > most common) and some opportunistic sub-behavior, like picking up food while moving if it exist in the passing tiles. We'll also add the first `fork()` related logic along our way, tied to `Idle` state. A bunch of things, but all bounded to `Behavior`, so we'll be fine. I'm sure.

Inserting priority and opportunity criteria in our Behavior is a matter of fine tunning stuff, which is to say know what to change with extreme specificity and try to do so without ending up with a broken state of things. Which is what happened to me a couple of times after editing the code. 

You see, encoding the priority order is not that complicated. We just need a `STONE_PRIORITY` constant to chec against and change how `computeMissingStones()` and `getNearestTileWithNeededResources()` work with the new priority in mind. Transitioning a priority-ordered loop in the first one makes it so the order in the class vector `_stonesNeeded` follows the fixed importance order. Then, instead of going through all the list and commit the target tile to the pure nearest, we switch to the **nearest with the resource with most priority**. This leaves us in a very specic logic state:
- There's room to add more complexity to this decision routine so that it is *wieghted*, wich means that it based on a ponderation of priority and closeness, so that the client doesn't target whatever it saw with a high priority when it is too far away in relation to some other thing that's considerable close but has a lowest priority. There's really no need for this, but it would be cool to do, so we'll toss it in the *maybe* box of our to-do list.
- The new priority focus navigation planification gives room for the opportunity layer, as now the little dudes might walk further, but we can take advantage of whatever resource is present in the passing tiles. That is: if there's food, just pick it, but also *if theres some non-targetted stone that turns out to be needed*, well, why not pick it up?

All of this has extended the `tickCollectStones()` function, which now looks like this:
```cpp
void Behavior::tickCollectStones() {
	if (_state.player.food() < FOOD_CRITICAL) {
		_aiState = AIState::CollectFood;
		clearNavPlan();
		return;
	}

	computeMissingStones();

	if (_stonesNeeded.empty()) {
		_aiState = AIState::Incantating;
		_incantationReady = false;
		clearNavPlan();
		return;
	}

	// pick up needed stone if already standing on one
	for (const auto& stone : _stonesNeeded) {
		if (_state.countItemOnCurrentTile(stone)) {
			clearNavPlan();
			_commandInFlight = true;
			_sender.sendPrend(stone);
			_sender.expect("prend " + stone, [this](const ServerMessage& msg) {
				_commandInFlight = false;
				if (msg.isOk())
					setInventoryStale();
				setVisionStale();
			});
			return;
		}
	}

	// Opportunistic food grab: don't interrupt a collection run, but take free food if it's right here
	if (_state.player.food() < FOOD_SAFE &&
		_state.countItemOnCurrentTile("nourriture")) {
		_commandInFlight = true;
		_sender.sendPrend("nourriture");
		_sender.expect("prend nourriture", [this](const ServerMessage& msg) {
			_commandInFlight = false;
			if (msg.isOk()) {
				_state.player.inventory.nourriture++;
				setInventoryStale();
			}
			setVisionStale();
		});
		return;
	}

	std::string previousTarget = _navTarget;
	auto tile = getNearestTileWithNeededResource();

	if (tile.localX == std::numeric_limits<int>::max()) {
		if (_navPlan.empty()) {
			std::vector<NavCmd> plan = Navigator::explorationStep(_explorationStep);
			_navPlan.assign(plan.begin(), plan.end());
			_navTarget.clear();
		}
	} else if (_navPlan.empty() || _navTarget != previousTarget) {
		std::vector<NavCmd> plan = Navigator::planPath(
			_state.player.orientation, tile.localX, tile.localY);
		Logger::debug("Behavior: CollectStones: planned " + std::to_string(plan.size()) +
			" steps to " + _navTarget + " at (" +
			std::to_string(tile.localX) + "," + std::to_string(tile.localY) + ")");
		_navPlan.assign(plan.begin(), plan.end());
	}

	if (!_navPlan.empty()) {
		NavCmd next = _navPlan.front();
		_navPlan.pop_front();
		executeNavCmd(next);
	}
}
```

One immediate consequence of this extension is that the single little dude probe is now winning the game considerably faster. From a general perspective, this is happenning because the current logic, after the above logged edits, is less "chopped", less compartimentalized around food and stones, flowing better in its navigation and picking stuff up along the carried out paths. A good sign, if you ask me.

Let's now focus on the initial fork logic injection. Nothing fancy, the only thing that needs some thought is *where* should fork be handled. Some design constraints around this (besides the decision around clients being able to fork multiple times or just one, irrelevant atm) are:
- Given that clients need to fork when in conditions are safe enough, how would we mark those? Easy: set up a `FOOD_FORK` thredshold, higher than `FOOD_SAFE` and, of course, quite higher than `FOOD_CRITICAL`. I'll set it to `20` (*for now, I must say, you might find out that the production files have a different value, even a higher amount of more varied food related constants*).
- Given that the set value is higher than the safe condition, the only place in the current behavior structure were a fork could actually happen is in `CollectStones` state. And because `tickCollectStones()` makes a food safe check at the top, then goes into proper stone search, a good place to inject this forking bussiness is right in the middle of those. This will give us the following flow when in `CollectStones` state:
	- Check if food levels are safe enough to continue in this state
	- Check if food levels are high enough to attempt a fork, and if so do so
	- Continue with the stone gathering logic towards achieving incantatation requirements
- Given that it doesn't really make that much sense to have level-1 clients forking themselves, will set the bar to at least them being level 2

Besides this, the fork call itself is nothing we haven't done quite a lot of times at this stage. We'll soon have to tweak some stuff regarding multi-client coordination, but for now this is more than enough:
```cpp
// TODO: decide if clients should have limited fork capabilities/shots or just fork whenever food is high enough
	if (_state.player.food() > FOOD_FORK && _state.player.level >=2 && _state.forkEnabled) {
		Logger::info("Fork call triggered");
		_aiState = AIState::Idle;
		clearNavPlan();
		_commandInFlight = true;
		_sender.sendFork();
		_sender.expect("fork", [this](const ServerMessage& msg) {
			(void)msg;
			_aiState = AIState::CollectStones;
			setVisionStale();
			setInventoryStale();
			_forkInProgress = false;
			_commandInFlight = false;
		});
		_forkInProgress = true;
		return;
	}
```
> *At this moment, I'm using `Idle` state as a pretty much useless track state for forking status. I'll get rid of this in the next steps, repurposing `Idle` to work towards multi-client leveling up and rallying.*

<br>
<br>

# 3.3 As the Crow Flies
Before moving on, I feel the need to take some time logging how pathfinding works in the current implementation (that is, without an A* algo, to which a transition might happen in the future). I write this from a near future, one that has once again found me in a failing loop of fixing-and-breaking things without achieving a two-client probe that reaches level 3 and has its players surive. Incredible stuff, I am so happy and fulfilled and I love life. Whatever, at least I now don't feel the need to rewrite everything from scracth, just the `Behavior`, which honestely is not going to be a *full* rewrite, but more like a *take-a-step-back-rethink-things-make-myself-recover-code-control-and-go-step-by-step-testing-things* kind of situation. And one very important thing in that backstepping is to take back navigation's reigns, as along the way I've been feeling progressively lost on my own implementation, which is twofold. And its twofold because the little dudes need two ways of navigating, related to two types of targets: *fixed, sure targets* like resources, *moving unsure targets* like rally leaders (more on those in the next point). Beyond this, the matter of the fact is that navigation in the context of this *Zappy* means taking care of, once again, two things: how to build the navigation logic, in the sense of how to make little dudes know where to go and how to get there, and how to make that navigation happen in the context of the server's ticking dyamic, which is to say how the navigation steps should be handled once they're built. To me, the first thing is easier, specially if this is not the first time you write pathfinding code (add to that the fact that pathfinding in a grid-locked system is not that complicated). The second one is a bit more intricate.

Because `Navigator` needs to work in an enmeshed system an embedded in a client-server logic that only allow for **one in flight command at a time**, the first careful consideration arrises quite soon: **the navigation system can never be "fire and forget"**. In other ones, it can't send a combination of, say, `TurnRight, Forward, Forward, TurnLeft, Forward` in burst and wait for all the command chain to finish. Which in practicallity means that if a navigation context called for that chain (or any other one, that is), the flow should be to send `TurnRight`, wait for the server's `ok` confirmation, then sen `Forward`, wait again for `ok` and so on. This is the reason behind `_navPlan`, the attribute bridgiing `Navigator` and `Behavior` being an `std::deque<NavCmd>`, a queue of pending moves related to the last built navigation plan, popped **only once per tick** by `Behavior`. The plan is the *memory* that `Behavior` has regarding what it still needs to do to get to a fixed target. Remember: always one-step-at-a-time. This is extremely important, you'll see why in a second.

`Navigator` has three static functions that build plans. There are no states, no side effects, nothing beyond these pure functions that just take some geometry and return a list of commands. Let's detail them

## 3.3.1 I've Made Plans and I Know Exactly Where I'm Going -> `planPath()`
When the client has spotted a specific tile in its vision and wants to walk there, it should call `planPath(facing, localX, localY)`. The X-Y values are the coordinates of the target tile in the **vision coordinate system**, which need to be translated into general, world-related coordinates based on their value and the `facing` direction of the client's little dude. The key thing here is that **this is a complete, pre-computed route from the current position to the target tile, computed at the moment of planning using the current orientation**. And even most importantly: **IT ASSUMES THE WORLD DOESN'T CHANGE DURING EXECUTION**. 

## 3.3.2 I Have No Target and I Must Move -> `explorationStep()`
Having the little dudes go stationary is not a good idea. They need food, they must effitiently gather resources, they constantly need to get new vision information of the state of the game and its landscape... Without movement, information quickly becomes stale, but clients won't always have a clear target or direction to make plans. Therefore, a fallback must be set in place, which our `Navigator` has in the form of `explorationStep(stepCount)`, a function that produces **ONE** (well, sometimes two) move: sometimes a turn, always a Forward. The turn pattern based on the 7-13 count criteria creates a loose spiral that covers the map without looping forver, and is taken as being *enough* because with a correctly configured server (that is, with rational density of resource production) clients shouldn't need too many ticks to find a purpose and, therefore, a target or direction. This function always returns a tiny plan, never a long route, because **exploratin is reactive**. Little dudes take a step, look again (refresh their vision), then make a new decision. There would be no point in planning further without knowing what's out there, beyond the level-restricted cone of vision of the little dudes.

## 3.3.3 Maybe It Was *Not* Just The Wind -> `planApproachDirection()`
The third navigation function is used to **move towards a sound**. Take this *sound* as something conceptual, understood as some little dude broadcasting something, i.e. *saying* something to the other memebrs of its team. This is only used in an specific state, `MovingToRally` (again, more on rallying in the next point), when, for example, a client with enough stones for a level-up ritual yells "HEY, FELLOW LITTLE DUDES, WE NEED TO GATHER IN THE SAME TILE TO ASCEND!!", to which the other clients need to respond and react. There's some mapping sub-processing here that is not very relevant (or it might be and maybe me thinking like this is one of the reasons of my continuous failure (in the project and in life)), needed because the server relies direction information in an 8-point basis which needs to be translated into a 4-point basis in the client (NSEW), but the important thing here is that **this function's target's position is never really known**. It only know **the direction in which the rallying call came**, and ever so roughly, as the emissor might move, disband, change ownership, who knows what else (I should know, I know, and I know, but I refuse to be sure about anything). Anyway, because this implied uncertainty, it only makes sense for `planApproachDirection()` to **return a one step plan** and wait for the next `RALLY` broadcast to update the bearing, then plan again. Something that, put like this, might sound like not a big deal, but when you get to the point of juggling states, resource counts, broadcasts, responses and timeouts, oh, lord, you're going to have some F U N. I guess that's the point of building this ai client, to learn how to manage onself in that context, but it bears the question of why would humanity see a lavish, flourishing world in front of it and decide that inventing 1)computers, 2)coding, 3)Zappy would be a good idea, something that makes cosmic sense, a weird way of finding meaning in this insignificant corner of an impossible to fully consider, ever-expanding, infinitely absolute universe. Maybe we're just a bunch of little dudes playing *Zappy* in God's twisted computer?

### 3.3.4 Execute Order <insert_number_here_I_refuse_to_write_66>
Let's get back to navigation management. One mayor understanding pitfall is the beforementioned fact that **the nav plan is not executed all at once**. Imagine a little dude plans a path to some tile and gets a 7 command chain back. The resulting `std::deque<NavCmd>` will have it's instructions processed one-at-a-time, per-tick based, in the following manner:
```cpp
_navPlan.assign(plan.begin(), plan.end());
// STUFF
NavCmd next = _navPlan.front();
_navPlan.pop_front();
executeNavCmd(next);
```
That is: adhere to a built plan, take out its next step, execute it. Impossible to think about a simpler procedure, but this calls for a careful consideration of the state a little dude finds itself after executing one navigation command, when the next tick of the server-client communication happens. So, at next tick, after the server responds to the sent navigation command, the little dude:
- Will have a stale vision, which means that it needs to refresh it. This happens because the vision information is situational, bound to the tile from which a `voir` command was sent, and moving or turning carries with it the effect of the previously captured vision no longer relating to the changed situation of the client.
- Will then tick its specific state
- The state-specific handler will see that there's an active, non-empty `_navPlan` (there are still 6 commands left)
- The next `NavCmd` will be popped and executed
- Rinse and repeat

Basically, there's **one server round-trip per command**, but **the plan persists across ticks**, shrinking by one command each time until its empty. Therefore, the loop can be destilled into: **"execute one step, get a response, execute the next step"**.

The way that stale vision's refreshment happens is by injecting it to the navigation command send callback. Everytime `Behavior` sends a navigation command via `Sener`, the registered command via sender's `expect()` function should `setVisionStale()` in the captured lambda function required as argument. This is what triggers `refreshVision()` at the top of the general `_tick()` entry point in `Behavior`. And gathering all of this around, we can define the rythm for a 3-step path like this:
```
Tick 1:  execute Forward  → commandInFlight
Tick 2:  response arrives → visionStale=true
Tick 3:  refreshVision    → commandInFlight
Tick 4:  response arrives → visionStale=false
Tick 5:  execute Forward  → commandInFlight
Tick 6:  response arrives → visionStale=true
Tick 7:  refreshVision    → commandInFlight
Tick 8:  response arrives → visionStale=false
Tick 9:  execute Forward  → commandInFlight
...
```

Ok, nothing to panic about up until this point, but there's one more caveat: **sometimes, plans need to be discardaded**. When, you ask? Well, let's talk about that.

## 3.3.5 I Love It When a Plan Comes Together, and I Must Learn To Love It When It Doesn't Too
Be can boil this into three distinct situations. The first one is the regular, all-good-chief one, in which we **keep the plan** because target is still valid and nothing has changed. This is usually the case when handling the `CollectStones` state, for example, as the target tile with the desired stone is not going to change. Well, the context *might* change in the sense that once the client arrives the stone might have been taking but some other client, but that's not relevant for the plan (and taking that into account is a refinement pass that is still a long way from us). The key condition here is that if we're at a state that's controlled/controllable as the stone gathering one and we have a `_navPlan` and the target of said plan has not changed, the plan is kept to avoid rebuilds on every tick for a multi-step journey. The implicit assumption here is that **the plan is still valid if the target type didn't change**. The plan was built from the tile's local coordinates at planning time, and those coordinates were correct for the orientation at that moment. Subsequent turns in the plan already account for re-orienting, so the remaining steps should still be correct.

Besides this, **plans are rebuilt** when:
- `_navPlan` is empty (exhausted or never started)
- `_navTarget != previousTarget` (a different stone became closest, or food was spotted mid-stone-collection)
- The target disappeared from vision (checked in `refreshVision's` callback):
```cpp
  if (!_navPlan.empty() && !_navTarget.empty() &&
      !_state.visionHasItem(_navTarget)) {
      clearNavPlan();   // target is gone, abandon the route
  }
  ```
- The move command failed (server returned `ko`):
  ```cpp
  } else {
      clearNavPlan();   // couldn't move, start fresh
      setVisionStale();
  }
  ```

And besides this, **plans should be immediately discarded when entering a new state or picking something up**. Whenever the behavior transitions to a new state, or picks up an item on the current tile (which removes the need to navigate to it), `clearNavPlan()` is called:
```cpp
// Entering CollectStones from CollectFood:
_aiState = AIState::CollectStones;
clearNavPlan();   // the food nav plan is irrelevant now

// Food on current tile:
if (_state.countItemOnCurrentTile("nourriture")) {
    clearNavPlan();   // no need to navigate anywhere
 
```

The rule here is simple: **a plan is only valid for the state that created it**. 

That being said, The current plan invalidation logic has a known limitation: **It only checks resource type presence, not tile-specific validity.** When moving toward a specific tile, the target resource might shift to a different tile in the vision (because client moved, changing the local coordinates of everything). The current logic will NOT clear the plan in this case, because `visionHasItem()` returns true (the resource type is still visible somewhere). This means:
- The plan may continue toward the original tile even though a closer tile with the
  same resource is now available.
- The plan will still succeed (the resource will still be at the original tile, because
  resources don't move), but it may take a longer path than necessary.

In this regard, a **potential improvement** could be to store the target tile's local coordinates at planning time,
and in `refreshVision`, verify that the specific tile still contains the target resource. If not, clear the plan and rebuild. And the easiest way to do so would be to expand `_navTarget` to a struct:
```cpp
struct NavTarget {
    std::string resource;
    int localX;
    int localY;
};
```
> *I might or might not do this. Check the code to find out. Relieve yourself from the mistery*

## 3.3.6 A Handful of Extra Considerations
I'll just bullet point my way through some specific knwoledge that I think needs to be logged:

### 3.3.6.1 The Role of `_navTarget`
This is just a string label (unless it has been changed into a struct) that records **what the current plan is navigating towards**. It seves two purposes: trigger change directions and vision-based invalidation.
- Before rebuilding a plan, `CollectStones` saves the old target name, re-evaluates the nearest needed resource, and compares.
	- If the nearest stone type changed, `_navTarget` will differ from `previousTarget` and the plan gets rebuilt

`_navTarget` is cleared whenever:
- A state transition happens and `ClearNavPlan()` is called
- An exploration step is used (there's no specific target to invalidate)

### 3.3.6.2 The Two Navigation Modes
There are really two fundamentally different modes of navigation, and they use the plan in completely different ways:
- **Mode A: Target-Directed Navigation**
	- This mode is used in `CollectFood` and `CollectStones`
	- The plan here can be multi-step, is computed once from the tile's local coordinates at te moment of planning, then trusted to be correct until it either finishes or gets invalidated
- **Mode B: Direction-Directed Navigation**
	- This mode is used in `MovingToRally` and exploration fallbacks.
	- Key difference is that the plan here is intentionally short and always rebuilt from scratch, because:
		- Exploration has no target, so there's nothing to preserve between steps
		- Broadcast direction changes with every Rally message

Really, in mode B the "plan" is really just a convenience wrapper to keep `executeNavCmd` the single dispatch point, taking a collection of commands.

### 3.3.6.3 The Vision-Stale Loop and Navigation
Here's the full loop written out explicitly, so the rythm is clear:
```
State handler wants to move → _navPlan is empty → call planXxx() → assign to _navPlan
→ pop front command → executeNavCmd(cmd) → send to server → _commandInFlight=true

[server responds]
→ callback fires → _commandInFlight=false → setVisionStale()

[next tick entry]
→ hasCommandInFlight()? NO
→ isVisionStale()? YES → refreshVision() → send "voir" → _commandInFlight=true

[server responds with vision]
→ callback fires → _commandInFlight=false → _staleVision=false
→ (possibly: target gone? clearNavPlan())

[next tick entry]
→ hasCommandInFlight()? NO
→ isVisionStale()? NO
→ isInventoryStale()? check...
→ dispatch to state handler
→ _navPlan non-empty? → pop front → executeNavCmd → repeat loop
```

Every single move has a `voir` injected after it. This is the "one-voir-per-step" pattern, put in place to keep the world model fresh at the cost of making navigation slow. Feel free to suggest any possible improvements on the production codebase.

### 3.3.6.4 The `MovingToRally` Special Case
`MovingToRally` is the most confusing state because it uses Mode B navigation but it **looks* like it might want Mode A. BUT, here's why it can't use mode A:
- `planPath` needs local `(x, y)` coordinates of the target tile, but the client doesn't know which tile the leader is on. They only know a compass quadrant.
- The correct tile to go to changes with every Rally broadcast. The leader's direction from the client updates as both leader an follower move.
- A client might need to cross the map's wrap-around boundary, which `planPath` doesn't handle.

This means that the follower does this instead:
```
Receive RALLY:2 with direction=3 (right quadrant)
→ planApproachDirection(3, currentFacing) → [TurnRight, Forward]
→ _navPlan = [TurnRight, Forward]
→ execute TurnRight → wait → vision refresh
→ execute Forward → wait → vision refresh

Receive next RALLY:2 with direction=1 (straight ahead now — we're closer)
→ clearNavPlan()  ← MUST happen here (this is the bug from fix plan Step 1)
→ planApproachDirection(1, currentFacing) → [Forward]
→ execute Forward → wait → vision refresh

Receive next RALLY:2 with direction=0 (we're on the same tile)
→ transition to Rallying
```

**The `clearNavPlan()` on direction update is critical here**: without it, the `TurnRight` and `Forward` from the previous step might still be sitting in `_navPlan` from a plan built for a now-obsolete direction, making the client walk the wrong way.

### 3.3.6.5 A Much Needed Summary
ere's the condensed decision table:

| Situation | Action |
|---|---|
| State transition to a new AIState | `clearNavPlan()` always |
| Item found on current tile | `clearNavPlan()` — no need to navigate |
| Target tile visible, no plan yet | Build full plan with `planPath` |
| Target tile visible, same target, plan non-empty | Keep existing plan |
| Target tile visible, target changed | Rebuild plan with `planPath` |
| Target tile gone from vision | `clearNavPlan()` (done in `refreshVision` callback) |
| No target visible | Use `explorationStep()` — 1-2 commands max, rebuild every step |
| Following a broadcast direction | Use `planApproachDirection()` — 1-2 commands, rebuild every RALLY update |
| Move command failed (`ko` from server) | `clearNavPlan()`, `setVisionStale()` |
| New RALLY direction received while in MovingToRally | `clearNavPlan()` — direction is stale |

**The master rule:** a nav plan is only valid for the state and target it was created
for, at the orientation it was created with.  Any change to any of those three things
is a signal to rebuild.

### 3.3.6.6 What Correct Navigation Trace Looks Like (I Think)
For a client facing North that needs to pick up a `linemate` at `localX=1, localY=2` (one right, two forward):

```
[tick]  visionStale=true   → refreshVision (voir)
[tick]  vision arrives     → tile (1,2) has linemate → plan = [TurnRight, Forward, Forward, TurnLeft, Forward, Forward]
[tick]  CollectStones      → pop TurnRight → sendDroite
[tick]  droite ok          → facing=East, visionStale=true → refreshVision
[tick]  vision arrives     → plan non-empty, target=linemate → pop Forward → sendAvance
[tick]  avance ok          → x++ → visionStale=true → refreshVision
[tick]  vision arrives     → plan non-empty → pop Forward → sendAvance
[tick]  avance ok          → x++ → visionStale=true → refreshVision
[tick]  vision arrives     → plan non-empty → pop TurnLeft → sendGauche
[tick]  gauche ok          → facing=North → visionStale=true → refreshVision
[tick]  vision arrives     → plan non-empty → pop Forward → sendAvance
[tick]  avance ok          → y-- → visionStale=true → refreshVision
[tick]  vision arrives     → plan non-empty → pop Forward → sendAvance
[tick]  avance ok          → y-- → visionStale=true → refreshVision
[tick]  vision arrives     → plan EMPTY → linemate on current tile → clearNavPlan → sendPrend
[tick]  prend ok           → inventoryStale=true, visionStale=true
```

> Six commands, twelve server round-trips, one collection.

<br>
<br>

# 3.4 Rally-Ho!