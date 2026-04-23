# Zappy Client - Devlog - 3

## Table of Contents
1. [The Hardest Choices Require The Strongests Wills](#31---the-hardest-choices-require-the-strongests-wills)
2. [Tightening the Screws](#32-tightening-the-screws)
3. [As the Crow Flies](#33-as-the-crow-flies)
	- [I've Made Plans and I Know Exactly Where I'm Going](#331-ive-made-plans-and-i-know-exactly-where-im-going---planpath)
	- [I Have No Target and I Must Move](#332-i-have-no-target-and-i-must-move---explorationstep)
	- [Maybe It Was *Not* Just The Wind](#333-maybe-it-was-not-just-the-wind---planapproachdirection)
	- [Execute Order <insert_number_here_I_refuse_writing_66>](#334-execute-order-insert_number_here_i_refuse_to_write_66)
	- [I Love It When a Plan Comes Together, and I Must Learn To Love It When It Doesn't Too](#335-i-love-it-when-a-plan-comes-together-and-i-must-learn-to-love-it-when-it-doesnt-too)
4. [Rally-Ho!](#34-rally-ho)

<br>

> *Just in case, Note: I've had to rewrite this part of the devlog several times. I hit a handful of walls, I wrote based on code that didn't end up working correctly, and because this is a somewhat hybrid log that gets written both during the process and after its fact, things got a bit messy. I say this because what's recorded in this document might give a came-here-to-learn-how-to reader a somewhat overwhelming feeling, steming from the straight-forwardness of the explanations. If that's your case, you can throw those feelings away, as everything written below has been rewritten and reviewed several times, and all the struggle that I had to fight during the process is not at all well reflected. Developing is never a straight line, and I spiraled quite A LOT while bulding this AI client. If you, too, struggle, just know the most important thing: it's normal. It should happen, even. That's how learning is, most of the times without our control, but what we can surely manage is our way of engaging with the process. I hope that, if you needed it, this documents help make your path a little bit easier. Besides that, trust me: been there, suffered that. Keep going.*

> *Note 2: because of the erratic aforementioned process, it might be the case that something in this document ended up with a wrong name or some other imprecise detail. Something like this or that constant said to be X but ended up being Y in the final codebase, or a name of a tracking boolean that was changed in its header class but not in its log section, ... I've tried to avoid having any of these, but just in case, know that some minor (and hopefully irrelevant) inconsistencies might arise while you read and cross reference with the production code.*

<br>
<br>

# 3.1 - The Hardest Choices Require The Strongests Wills
After reaching or first milestone, we can be sure (or as sure as we can be) about having a working base that 1) acts as a checkpoint in case Charles The Bot The Third kicks the bucket, and 2) makes the road ahead a pure matter of extending. This means that our next milestone, which is to have a single little dude reach max level by itself in easy mode, as well as anything that comes later, is just going to ask us to work on adding layers of possibility and complexity to `Behavior` and `Navigator`. Tools and pipelines for stone gathering, rallying and forking, those things that are still pending between us and a full, non-easy winning game bot. I trust in Charles III, but we'll have to see if it believes in itself.

So, as I said, the first goal going forward is to make our little dude gather stones, Thanos-style, and our first small step should be to make our probed single little dude to reach level 2, as the 1→2 transition is the only one that doesn't require aditional participants beyond the S E L F. Doing so is going to require:
- New `CollectStones` and `Incantation` states added to the enum of the machine
- A table of stone and player requirements for each level transition (we'll only use the first one for now, but we'll need it for the next step so let's get it out of our way)
- Create transition patters between them
- Maintain a safe lock for emergency food levels (i.e., triangulate these two new states with the already existing `CollectFood`)
- Add navigation path searching targetting stones
- Add incantation triggers and managers

That sould do it. And with or new list in our hands, let's get into implementations. First, and after extending the `AIState` collection, we'll expand `Behavior`'s attributes to track its `AIState`, and three level-up related additions (`_stonesNeeded`, `_incantationReady`, `_stonesPlaced`) to track the progress of the ascension ritual. After this, our next important change is related to how `tick()` works in `Behavior`. Up until this point, our little dude has just been following a fixed pattern without regards to its state, but we have to transition into a proper state-machine like state-depending ticking. To do so, we'll refactor `tick()` to be an entry point to the ticking process, containing just a switch case on the current `AIState` in order to derive execution to specific ticking sub-functions, all while keeping the existing top-checks. Something like this:
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

> *PAUSE: This is me from the future reading me from the past, and thinking "oh sweet summer child", and wanting to give a heads-up: the collection of ticking functions will grow significantly, following the accumulation of different AI states that will make up the final Behavior of this client. Building a state machine with 9 states is not the simplest thing in the world, but not because its code being complex, but because the transition and state-related specific management can be hard to mentally track. At least in my case, I'd say that this was the main battlefront of the war: wiring states between each other, defining gateways with utmost precision, knowing when a state, or even more, a specific path in every state, needs to be doing is complicated. A big sack of trials and errors, from the ground up, as the most basic decisions like "when, why, in what states and what paths should the AI require a fresh vision and/or inventory can be sort of tricky. Failure to correctly handle this can lead to decisions made based on stale information (disaster), and excessive calls to these refreshes can end up slowing, even stalling behaviors, sending little dudes in loops of just VOIR VOIR VOIR. And this times every little nook and cranny of the AI (when to clear an unfinished navigation path? When to maintain an in-flight command? When should a collection of flags related to specific sub-routines be reset? Etc.). So, yeah, the small `tick()` shown above is a much more simpler configuration that what will be needed by the end of this process, bare that in mind, go step by step. Avoid tackling the production state of things in the codebase as what you should write head-on. That is Not the way.*

Now, for the stone collection behavior, housed in `tickCollectStones()`, we'll need to do the following:
- For safety, we'll first check if while searching for stones food reservers went below the critical threashold, and if so we'll have the AI transition its state back to `CollectFood`
	- We'll make this check against `FOOD_CRITICAL` and not `FOOD_SAFE` because if we did so, we'll have a little dude ping-pong-ing between states, looping endlessly, lost forever.
- Compute missing stones
- If there are no missing stones, state will transition to `Incantating`
	- *This is just, and will remain as, the case for a level 1 little dude. The rest of the level incantations, because they need more players and therefore they must go through a rally, will have more restrictions set up in the transition gateway. More on that later.*
- Else we'll have our little dude find the nearest tile with a needed resource, prioritizing distance over anything else (i.e., the check must be done tile-based, not missing-stone-based)
- After finding it, we'll have the little dude plan a stone-targetting navigation

Writing this logic is not a big deal. At the end of the day, `tickCollectStones()` is one of those flow functions in which what's really important is the order of checks and operations. The AI needs to know what stones it needs to search for, and once it knows what it is looking for, the logic is practically the same as looking for `nourriture` items. So, really, the important stuff here is the implementation of a couple of helpers, `computeMissingStones()` to acquire stone targets, and `getNearestTileWithNeededResourcer()` to locate them.

`ComputeMissingStones()` is just a checkup against the pre-made table of requirements per level transition. A `LevelReq` struct placed in `Behaviour.hpp`, tracking the amount of players for an incantation ritual, and the specific amount of stones (as an `Inventory` object) suffices to set up a static function that creates the mentioned table. Then it's just a question of pure comparison and cross-referencing:
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
	
	// easy mode -> 1 linemate is enough
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

> *You'll see right now that, if you check out this function in the production code, there's one extra block in it regarding a level 1 check. This was added because incantation rituals require for stones to be in the celebration tile, so picking the single linemate resource neded for a 1->2 level up, just to then have to place it back in the floor is both a waste of time and a choke-point that can halt initial progression because of stone juggling between clients and teams. The added block is this:*
```cpp
if (_state.player.level == 1) {
		if (_state.countItemOnCurrentTile("linemate") < 1) {
			_stonesNeeded.push_back("linemate");
		}
		return;
	}
```

This all that our little guys need to roam around and gather stones, wrapped around some state transitions that take them back to food gathering if they go below the emergency threshold, and go forward into `Incantating` state if the set of required stones are in their inventory. A couple of notes here, though:
- When I first wrote `tickCollectStones()`, I considered the stones in the current tile of the little guy as part of a total sum for the requirement comparison. This didn't work properly. There's risks to doing so because the client might pass, say, a linemate check because there's linemate in its current position, but after detecting that it needs to move to get some deraumere that check would fail, and maybe this happens after confirming that deraumere is in place, but not because it was picked but because the target, deraumere-containing tile was reached... And so on and so forth. I guess that there must be a way to consider the stones already existing in tiles to make the whole game-playing process more optimal, but it feels like it would be difficult to pinpoint it and not worth the hustle. So, for now and most likely forever, the requirements for a level up transition will only be checked against the client's inventory: when a little dude has the necessary stones in its pocket, he will try to transition into Rallying (then it will be assigned as leader or follower for the Rallying process, then maybe advance to incantation).
- At this stage/milestone, we're just working with a single little guy, which can reach max level and win the game if the session is set up in easy mode (via the env variable `ZAPPY_EASY_ASCENSION=1`). Until we transition to the next milestone, concerning multi-client cooperation, special considerations regarding the "role" a client assumes when going into `Incantating` state are irrelevant, i.e. not yet implemented. Maybe I'll have to set up class attributes to track the existance and identity of a "leader", I don't know. We'll see. 

> *I already saw, and yes: there was in fact a huge necessity of a bunch of attributes to set up the multi client logics, just FYI.*

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

`tickIncantating()` is going to be a little bit trickier, though. But we can do it, remember, we have a system for tackling things: let's think about what this needs to do. Everything we need is in one of these places: past logs, our head, our heart, God. So we just have to connect with the correct source of knowledge to know that the function is going to need the following steps:
1. Be sure that it's working in a non-stale vision state, triggering a `refreshVision()` call before moving on
2. Place the required stones for the level transition in the floor (as stated by the rules of the game).
	- This is the most tricky part, really, but deep down is just a thorough cross-check between the transition requirements, the tile contents and the little dude's inventory, which has to halt incantation ticking progress until all stones are in place.
	- With a single client in easy mode this is non-important, as every incantation just needs 1 linemate. But in any other context, the RALLY->INCANTATION process will go through the leader set up, and the leader will use the time needed by followers to reach him to place the collected stones in the incantation tile.
3. The third step is a safety check wrapping the previous step, a way of being 100% sure that the tile contains the necessar stones
4. Once the previous check passes, `_sender.sendIncantation()` can be fired, with some detailed handling of the callback sent to the `expect()` function, as incantation is a two step process in the server: it get's requested and aknowledge first, it gets resolved second.
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

# 3.2 Tightening the Screws
Our next milestone is to extend the stone gathering behavior so that **every level requirements are correctly gathered** in a non-easy context. We'll have to be careful to not break the 1→2 trasition (which little dudes can do by themselves), but we'll have to test level after level if the clients target the rocks the are supposed to. We'll do so while also injecting some resource type priorities (rarest > most common) and some opportunistic sub-behavior, like picking up food while moving if it exist in the passing tiles without renouncing the main resource objective and the related, if existing, navigation plan. We'll first focus on client cooperation and leave `fork()` related stuff for the end of development, so our testing probe for this stage will be a **non-forking, single-team, multi-client probe that survives and reaches level 8**. A bunch of things, but all bounded to `Behavior` and `Navigator`, so we'll be fine. I'm sure.

> *Basically: we need to think about efficiency early on, at leat to some extent, because surviving alone in easy mode is trivial, but surviving in an strict, coordination-requiring context needs a finer tune. It would be reckless to have little dudes gathering stones all around without consideration to opportunity and self-preservation, not to mention them being able to make at least some informed decisions regarding where to go when*.

Inserting priority and opportunity criteria in our Behavior is a matter of fine tunning stuff, which is to say know what to change with extreme specificity and try to do so without ending up with a broken state of things. Which is what happened to me a couple of times after editing the code. 

You see, encoding the priority order is not that complicated. We just need a `STONE_PRIORITY` constant to chec against and change how `computeMissingStones()` and `getNearestTileWithNeededResources()` work with the new priority in mind. Transitioning a priority-ordered loop in the first one makes it so the order in the class vector `_stonesNeeded` follows the fixed importance order. Then, instead of going through all the list and commit the target tile to the pure nearest, we switch to the **nearest with the resource with most priority**. This leaves us in a very specic logic state:
- There's room to add more complexity to this decision routine so that it is *weighted*, which means that it would be based on a ponderation of priority and closeness, so that the client doesn't target whatever it saw with a high priority when it is too far away in relation to some other thing that's considerable close but has a lowest priority. There's really no need for this, but it would be cool to do, so we'll toss it in the *maybe* box of our to-do list.
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

<br>
<br>

# 3.3 As the Crow Flies
Before moving on, I feel the need to take some time logging how pathfinding works in the current implementation (that is, without an A* algo, to which a transition might happen in the future). I write this from a near future, one that has once again found me in a failing loop of fixing-and-breaking things without achieving a two-client probe that reaches level 3 and has its players surive. Incredible stuff, I am so happy and fulfilled and I love life. Whatever, at least I now don't feel the need to rewrite everything from scracth, just the `Behavior`, which honestely is not going to be a *full* rewrite, but more like a *take-a-step-back-rethink-things-make-myself-recover-code-control-and-go-step-by-step-testing-things* kind of situation. And one very important thing in that backstepping is to take back navigation's reigns, as along the way I've been feeling progressively lost on my own implementation, which is twofold. Twofold because the little dudes need two ways of navigating, related to two types of targets: *fixed, sure targets* like resources, *non-position unsure targets* like rally leaders (more on those in the next point). Beyond this, the matter of the fact is that navigation in the context of this *Zappy* means taking care of, once again, two things: how to build the navigation logic, in the sense of how to make little dudes know where to go and how to get there, and how to make that navigation happen in the context of the server's ticking dyamic, which is to say how the navigation steps should be handled once they're built. To me, the first thing is easier, specially if this is not the first time you write pathfinding code (add to that the fact that pathfinding in a grid-locked system is not that complicated). The second one is a bit more intricate.

Because `Navigator` needs to work in an enmeshed system an embedded in a client-server logic that only allow for **one in flight command at a time**, the first careful consideration arrises quite soon: **the navigation system can never be "fire and forget"**. In other ones, it can't send a combination of, say, `TurnRight, Forward, Forward, TurnLeft, Forward` in burst and wait for all the command chain to finish. Which in practicallity means that if a navigation context called for that chain (or any other one, that is), the flow should be to send `TurnRight`, wait for the server's `ok` confirmation, then sen `Forward`, wait again for `ok` and so on. This is the reason behind `_navPlan`, the attribute bridging `Navigator` and `Behavior` being an `std::deque<NavCmd>`, a queue of pending moves related to the last built navigation plan, popped **only once per tick** by `Behavior`. The plan is the *memory* that `Behavior` has regarding what it still needs to do to get to a fixed target. Remember: always one-step-at-a-time. This is extremely important, you'll see why in a second.

`Navigator` has three static functions that build plans. There are no states, no side effects, nothing beyond these pure functions that just take some geometry data and return a list of commands. Let's detail them

## 3.3.1 I've Made Plans and I Know Exactly Where I'm Going -> `planPath()`
When the client has spotted a specific tile in its vision and wants to walk there, it should call `planPath(facing, localX, localY)`. The X-Y values are the coordinates of the target tile in the **vision coordinate system**, which need to be translated into general, world-related coordinates based on their value and the `facing` direction of the client's little dude. The key thing here is that **this is a complete, pre-computed route from the current position to the target tile, computed at the moment of planning using the current orientation**. And even most importantly: **IT ASSUMES THE WORLD DOESN'T CHANGE DURING EXECUTION**. Not that it will matter down the line, as every move calls for a vision refresh, but it will make things easier when writing the function.

## 3.3.2 I Have No Target and I Must Move -> `explorationStep()`
Having the little dudes go stationary is not a good idea. They need food, they must effitiently gather resources, they constantly need to get new vision information of the state of the game and its landscape... Without movement, information quickly becomes stale, but clients won't always have a clear target or direction to make plans. Therefore, a fallback must be set in place, which our `Navigator` has in the form of `explorationStep(stepCount)`, a function that produces **ONE** (well, sometimes two) move: sometimes a turn, always a Forward. The turn pattern based on the 7-13 count criteria creates a loose spiral that covers the map without looping forver, and is taken as being *enough* because with a correctly configured server (that is, with rational density of resource production) clients shouldn't need too many ticks to find a purpose and, therefore, a target or direction. This function always returns a tiny plan, never a long route, because **exploratin is reactive**. Little dudes take a step, look again (refresh their vision), then make a new decision. There would be no point in planning further without knowing what's out there, beyond the level-restricted cone of vision of the little dudes.

> *As a side note, this function was quite helpful for troubleshooting navigation during states that shouldn't use it. For example, I was having follower clients in `MovingToRally` state (that is, moving to the Leader's tile) falling back to `explorationStep()`, easily identifiable via logging, which immediately flagged a problem in the logic. Because a good implementation would mean that there would never be a follower without a leader, the fact that a follower was having to result to move without a direction was a clear sign of a bug*.

## 3.3.3 Maybe It Was *Not* Just The Wind -> `planApproachDirection()`
The third navigation function is used to **move towards a sound**. Take this *sound* as something conceptual, understood as some little dude broadcasting something, i.e. *saying* something to the other memebers of its team. This is only used in an specific state, `MovingToRally` (again, more on rallying in the next point), when, for example, a client with enough stones for a level-up ritual yells "HEY, FELLOW LITTLE DUDES, WE NEED TO GATHER IN THE SAME TILE TO ASCEND!!", to which the other clients need to respond and react. There's some mapping sub-processing here that is not very relevant to us because it's done in the server side. But if you want/need to know, the direction from which a sound (a broadcast) comes needs to be computed by considering the placement of the emissor and the placement AND orientation of the receiver, a process which you can see laid out in [game.c](../server/srcs/game/game.c), around `line 1033`, inside `compute_broadcast_direction()`.

What really matters: **this function's target's position is never really known**. It only knows **the direction in which the rallying call came**, with the safeguard (set up by our code) that the source of the sound will never move, because our broadcasting leaders will stay put while doing so, and disband themselves and give up leadership if they need to move for any reason, mainly starvation closing in. Anyway, because this implied uncertainty, it only makes sense for `planApproachDirection()` to **return a one or two step plan** and wait for the next `RALLY` broadcast to update the bearing, then plan again. Something that, put like this, might sound like not a big deal, but when you get to the point of juggling states, resource counts, broadcasts, responses and timeouts, oh, lord, you're going to have some F U N. I guess that's the point of building this ai client, to learn how to manage onself in that context, but it bears the question of why would humanity see a lavish, flourishing world in front of it and decide that inventing 1)computers, 2)coding, 3)Zappy would be a good idea, something that makes cosmic sense, a weird way of finding meaning in this insignificant corner of an impossible to fully consider, ever-expanding, infinitely absolute universe. Maybe we're just a bunch of little dudes playing *Zappy* in God's twisted computer?

> *Another side note: some past versions of this function returned more than one forward call in the navigation plan in an attempt to reduce navigation times. This was a huge mistake, the perfect recipe for having little dudes overshoot their traversing and having to take 20 steps towards a 3 tile near objective. Bad, bad, bad. Don't do it*

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
We can boil this into three distinct situations. The first one is the regular, all-good-chief one, in which we **keep the plan** because target is still valid and nothing has changed. This is usually the case when handling the `CollectStones` state, for example, as the target tile with the desired stone is not going to change. Well, the context *might* change in the sense that once the client arrives the stone might have been taking but some other client, but that's not relevant for the plan (and taking that into account is a refinement pass that is still a long way from us). The key condition here is that if we're at a state that's controlled/controllable as the stone gathering one and we have a `_navPlan` and the target of said plan has not changed, the plan is kept to avoid rebuilds on every tick for a multi-step journey. The implicit assumption here is that **the plan is still valid if the target type didn't change**. The plan was built from the tile's local coordinates at planning time, and those coordinates were correct for the orientation at that moment. Subsequent turns in the plan already account for re-orienting, so the remaining steps should still be correct.

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

And we'll continue in the next log, I think both you and I have had enough for now and need a break.