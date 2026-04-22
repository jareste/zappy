# Zappy Client - Devlog - 2

## Table of Contents
1. [Charles the Bot the Third (a.k.a. Carlitos III)](#11---there-and-back-again-in-a-network-sense)
2. [Little Dudes and Their Little Food](#22-little-dudes-and-their-little-food)
    - [You've Got Mail (Read in Elwood Edward's Voice, AOL Style)](#221-youve-got-mail-read-in-elwood-edwards-voice-aol-style)
	- [Like Duolingo but for Net Communications](#222-like-duolingo-but-for-net-communications)
	- [Commands, Fall In!](#223-commands-fall-in)
	- [State of the Union](#224-state-of-the-union)
	- [On Our Best Behavior](#225-on-our-best-behavior)
	- [Sir, this is a Wendys](#226-sir-this-is-a-wendys)
	- [Agent Little Dude](#227-agent-little-dude)
	- [Is It Alive?](#228-is-it-alive)


<br>
<br>

# 2.1 - Charles The Bot The Third (a.k.a. Carlitos III)
What's that? You had a working network code a couple of weeks ago and you have been struggling to build a functional, surviving, game-winning-capable AI client since then, hitting several walls along the way, crying in your sleep, pulling each and every hair in your little, so limited head, arriving to a desperate situation after two failed builds, and are grasping piping hot nail of that old saying, "third time's the charm", telling yourself that this time will be different, that this is the one, that Charles the Bot the Third will thrive, live, play the game, win the battle, end this everlasting spiral of failing probing tests that has become your life for you don't even know anymore how many days? Ah, you see, I've been there, what a coincidence. You, as me, might have come to the conclusion that wiring up an AI for this *Zappy* hell whole is more complicated than what you thought. After all, you just need your little dudes to go around a grid, collect food and weirdly named stones, level up, keep going and win. What could be so complicated about that? Ah, haha, well, oops. At least we've got that good old, unbreakable stance: never retreat, never surrender. You and I, together, will achieve our goals. This too shall pass.

Beyond the self-pity (which we're going to turn into self-compasion!!), these past days of failure have been quite enlightening towards the specific subject at hand. I haven't arrived to a working AI, but I've arrived to several reasons for it *not* to work, which helps. The first build taught me how to balance the client's need for survivable (i.e., for food gathering) and ambitions (i.e., for stone gathering and leveling up). Making sure that the little dudes kept eating while searching for the necessary stones requires fine tuning of priorities, which in itself mean that the client needs to handle a complex reality: **it needs to navigate contingency**. Take this sequential situation:
- One little dude has enough food to go past the safety threshold, so it goes into stone gathering state
- It sees that close to him, in some tile covered by his vision, a unit of the stone he needs is pickable, and plans a serie of navigation steps towards it.
- While navigating, the server ticks, getting food out of his inventories, making his reserve amounts go below the safety threshold
- Now, little dude needs to decide: Should I switch to feeding mode? Should I keep going because I'm still some food units away from the danger threshold? Should i YOLO it?
- Then, imagine that because the path was somewhat long, little dude needs to go into eat-or-die panic mode, sends a vision request to the server, finds food around, re-plans his path to reach that food
- But while going to get that food, some other little dude arrives first and takes the targeted unit, with the particul2.2 Little Dudes and Their Little Foodarity that there was only one unit of food in that tile, so your little dude's path to the life-saving tile is no longer valid
- Little dude now needs to reassess its surroundings, build a new path, continue...
- And so on and so forth

Easy to get into words, yes, but not so easy to get into code, specially when you have to test things by probing client-server interactions, check stuff via logs, changin one little knot of the decision net, repeating. With just one little guy, could be more or less *simple* (not really), but if want to test anything beyond a level 2 little dude, you're going to have to go into multi-client probing, which is a fast-forward alley to shit hitting the fan. I must stress that, with all of this, I'm stating that coding an AI for this *Zappy* is one of humanity's greatest achievements, just that for little old me, who has not that much experience in building things like this, finds it challenging (quite), so please don't be mad at me. Now, the multi-client aspect can be overriden by testing in "easy" mode, which makes the game dynamic of leveling up get rid of the player amount requirement for a level up incantation to succeed, but this context only gets you a couple of steps closer to the complex AI we have as objective. Mostly, you'll be able to test the solidity of your Navigation and make a partial assessment of how incantation is working, mainly regarding how it's handled in the comunication layer between client and server (i.e., that your client saying "hey I'm ready and want to level up", and your server replying "let me check, ah yes, you're right, you're now leveling up", and the client ending the process by aknowleding that it is now a one more level little dude).

From there, the multi-client context will soon, and in my case horribly painfully, make you realize that the clients don't only need to survive, know how to navigate the game world and collect the specific stones they require for their level situations: **your little dudes are going to need to organize themselves**. And if this realization meets you after building a substantial chunk of the AI codebase, you might find yourself in some trouble. See, by "organizing themselves" what I mean is that the little dudes need to **rally** themselves around complex incantations to go beyond level 2, as past that level the requirements always contain a specific amount of players (for example, you need 2 level-2 little dudes celebrate the level-3 incantation ritual, which will take those two guys into the next level, and so on with the following ones, with specific needs stated in the project's instructions). So, again, let's consider a sequential situation:
- One little level-3 dude has gathered the necessary stones for a 3->4 ritual, and now needs two other level-3 little dudes to join him in a tile to celebrate the incantation.
- So, this little dude broadcasts: "Hey, fellow little dudes, we can now try to ascend, come to me!", and this broadcast is received by two other level-3 dudes, containing the position of that little dude that has now become a **leader** in the ascension process.
- BUT, while our followers travel to the leader's position, said position becomes stale because the leader needed to move in order to keep a healthy reserve of food, which means that the leader is going to have to keep broadcasting its position so that the followers don't lose track of him, all while taking into consideration their own food need.
- Eventually, and at least ideally, our wanting-to-ascend little dudes meet up and go into the next step, which is to drop the incantation stones in the tile (via `pose` commands) in the celebration tile. And maybe, while doing so, hunger's sharp claws strike some of the participants, who now have to decide what should take place: should they go for food and risk a ritual desync? Should they stay and maybe give up their lives for the ascension? After all, what is more valuable for a team, keep 3 level-3 little dudes alive or sacrifice a couple of them for a couple, or even a single level-4 little dude?
- And all of this without taking into consideration other factors that could take place into the go-into-incantation collective decision making, like:
    - A little dude having almost enough stones for an incantation and pre-broadcasting a rally in a tile where the missing ingredient already exist, which will come with a risk of said ingredient being taken away in the mid time (a very competitive AI landscape would *attack* this situation, wouldn't it?).
    - A collective of little dudes having the necessary stone between them, realizing so, rallying themselves up (should this layer of complexity be implemented?)
    - And so on...

As you can see, this is not simple at all, or at least I don't consider it to be so. And all this details showing up when working with what reveals itself as a bad base to embrace them can quickly lead to the worst realization of all: "I should start over". Which is what happened to me and tragically ended the life of Charles the Bot the First and birthed its successor. I imagine the second AI codebase running for the first time, realizing it has "II" in the name and wondering why, where's "I", what happened. Oh, my cute disfunctional bot, you don't want to know, you don't want to know...

As you already know (or could infer from the title of this 2.1 section), Charles the Bot the Second didn't make it either. It is now in Bot Heaven, having the zoomies in green, Bot Pastures, eating yummy Bot Food, being with its fellow Bot Angels. Why? Well, this is more difficult to pinpoint and condense in reasons as specifics as those written above. With Charles II probes were having better progress, I even reached a point in which some clients were arriving at level 4, but going beyond that seemed like something that was not going to happen, no matter what edits I made to a progressively more ghoulish codebase. If I tweak priorities and decision paths in some place to help teams ascend to higher levels, dying bots piled up. If I wrote countermeasures to that so that they had a more robust behavior towards food, long ass probes resulted in, at most, a couple of level-3 clients roaming around a stale game session. I wasn't being able to fix anything without breaking something else because I had some pathfinding issues, some mismatches in how orientation was handled between client and server (which at that point moved me towards a conversion pipeline, trying to avoid what anyways happened, a second decision to rewrite everything), rallies were not working, the patch I introduced to handle in flight commands in a client came too late to be meaninfull in any way in what was already a poison swamp of code, stale management of vision result became a huge pitfall, and, as I realize now that I'm halfway my third attempt, my broadcast pipeline was too ambitious to be sustained by the weakest of foundations.

So, yeah, Charles II is now in bot heaven with its antecessor, god bless their little, artificial souls, and you finde me hands deep in the mechanical heart of Charles III, which I hope doesn't need to take that jourey upwards, staying this side of that river that we all have to cross at some point. That's the sad part of all of this: giving life to a bot means also giving it death. We build to only just destroy what we birthed. We try to code reason to little dudes we throw into a meaningless, pretend world. We're horrible people with grandeur complex, drentched in hubris, telling ourselves that keeping fed and gathering stones is enough. At some point, we as humans made rocks into thinking structures built to play a game about gathering stones. I'd say that only God could judge us, but he made us this way, so the inevitable question (inherited, really, old as History) is one with an answer well beyond the limits of our comprehension: who will put God into trial? Who will judge the Judge?

Anyway, we are still One Functioning Bot short, and this third time I'm going to restrain myself of going head in (butt in, really) into coding. I'll document the process and go step by step, very slowly, as steady as I can, trying to make myself sure that whatever milestone I reach is as correct as I can possibly can, all while unit-testing each implementation along the way.

<br>
<br>

## 2.2 Little Dudes and Their Little Food
Knowing what we know, harnessing the immense power that our failures grant us, we can establish a first big milestone for Charles the Bot the Third: existing, correctly navigating, keeping itself fed, all while knowing that down the line little dudes need to cooperate with their fellow clients. As every devlog I've written this far, I'd like this to work twofold, both as a diary to me (and a reminder of what the hell I programmed for future, SURE TO BE HAPPIER, RIGHT??? me), and as some sort of guide to the fellow suffering programmer that might find themselves in a similar situation as mine and, for some twisted reason, ends up reading this. I'm going to document the process of Charles III, which at the exact instant of writing this line is still quite far from being done, but I don't want to have to write documentation from scratch for a too big of a codebase that makes me don't want to do it and, well, not do it. I hope this works for you if you are one of those fellow sufferers. 

So, gathering pain and past lessons, I'd say that if you want to write a *Zappy* AI bot's game logic (given that the net code is, at this point, taken as done and working, if not I'd suggest going to [the previous log](devlog_01.md)) the best way to split the code is to have a `Protocol` and an `Agent`, all managed by a very simple `Main`. That's it, and it could feel like too little, but trust me: you want to keep things simple were you can yo make room for complexity where it is unavoidable. Going into more depth:
- `Protocol`: this wing of the codebase handles the client's side of communication management, regarding how messages are received and sent. It needs the following, or some equivalence:
    - `Message`: pure data. NO parsing logic, no methods beyond helpers that might be needed to access the data, just a way to store the wide array of information that can be sent from server.
    - `MessageParser`: the in-between piece at the client-server touching point, converting server JSON messages into client data structs.
    - `Sender`: the `WebsocketClient` wrapper that is in charge of sending commands to the server and handling the returned responses.
- `Agent`: the client itself, the section of the client codebase that knows about the game world and, therefore, can exercise a Behaviour towards its state, make decision and build navigation paths. It needs the following, or some equivalence:
    - `State`: pure data. Owned by the agent class, stores data structs to track the player and the world state, the latter containing, at most, quering methods.
    - `Navigator`: a collection of functions that build navigation commands from world state information.
    - `Behavior`: the AI state machine. Sits between the process of reading the world state and the issuing of commands via the sender. Contains the logic that tells the client how to act, and connects with the tools to make it happen.
    - `Client`: owns the main loop and connects all layers together. It is JUST A MANAGER, i doesn't contain any game logic, it does not make any decision, it does not parse messages. Think of it as the orchestrator which has two main *regions* of action:
        - Related to net
            - Connects to the server via `WebsocketClient`
            - Handles login sequence
            - Handles reconnections on disconnect
        - Related to the main loop
            - Calls the websocket ticking function on every loop iteration
            - Calls the timeout check function in the websocket
            - Ticks the behavior class
            - Routes incoming messages
- With this structure, `Main` only needs to handle a couple of initial responsabilities:
    - Parse CLI arguments
    - Setup signal handlers
    - Create the AI `Agent`
    - Call the agent's connect pipeline
    - Call the agent's `run()` pipeline, which blocks main until done, or until an interrupting signal is sent (`SIGINT`/`SIGTERM`)

To the best of my knowledge, this creates a suffient, manageable structure that can grow in functionality, scope and complexity as implementations take place in the codebase. There are no circular dependencies, each element/layer can be unit-tested in isolation (mocking, when necessary, the layer below it via the mock utilities given by the chosen gTest based suites) and, most of all, is strongly compartimentalized. The conceptual result is an agent that has three main structural points: **the Net Code, the Protocol code and the Agent code**. Nothing more, nothing else, but don't let your guard down: the fact that we can boil down the client into this little amount of pieces doesn't mean that this is going to be an easy to build, nor tame, codebase.

After this initial structuring step, what we need is a first milestone comprised of multiple, testeable steps. This first milestone will take us to a first working AI agent with basic functionality. It will:
- Connect, handshake and login the server
- Start its game loop
- Survive hunger-wise, i.e. move around the game arena to gather food and avoid starvation.

As previously stated, we *know* things, we're aware of a handful of possible pitfalls that will appear down the line if we don't build with future complexity in mind, so we'll be extra careful while treading towards this first milestone. This, on the other hand, doesn't mean that as we create files and write classes they need to work on the get go with the full scope of said complexity in place, just that *some things* need to be done with full awareness that they will change, be extended or transform in meaningful ways as development progresses. Except for the last steps in this first milestone's roadmap (which I have in another document; as I mentioned above, I'm already at that milestone while writing this part of the devlog), the steps that lay down before us are mainly based in creating, defining and testing the elements listed above, with just some tweaking in the `Behavior` after the `Navigator`'s basics are written (mainly because I wanted to have a very basic, hardcoded behaviour first to test some of its aspects, before wiring it to the initial, food-seeking navigation logic offered by the initial iteration of the `Navigator`). Let's get to coding.

### 2.2.1 You've Got Mail (Read in Elwood Edward's Voice, AOL Style)
We'll start from the very beginning, client wise. At this point we have a codebase that lets us build a client that connects to the server and open a communication channel. If we think about the program's overall process, we can boil it down, at this point and on this side, to a server sending messages to the client and the client sending messeges to the server, among which things *happen*. So we should start by building the necessary data struct on the client side to store the server's communications, which have a somewhat wide array of possibilities in regards to their contents. And precisely there is where our first question lives: **what is the server sending the client**?

Well, knowing the game we're playing and having access to the server codebase, we can make some decisions:
- Because the clients/players need to hold data of what the see and what they have, we're going to need something in the likes of `VisionTile` to store a collection of tile-related data (the amount of tiles seen by a player is level-dependent) and an `Inventory` to track what possessions.
- Because there are different types of messages sent from the server, we're going to have to handle different execution paths for each of them, so the client is going to need a `MsgType` enumerator to have an equivalence to the server's fixed tipes.
- And, of course, we'll need a general data struct to store a received message, which will be treated as a packet/payload of varying information. A `ServerMessage` struct that holds the message type, its raw contents, some queries, and makes an explicit differentiation of what data is mandatory and wht is optional
    - `cmd`, `arg` and `status` are mandatory. Server's communications will always contain those, although some of them, like status, could be empty given the case.
    - Command specific related data is always optional. For example, a message ***can*** contain information about `mapWidth`, `mapHeight`, `remainingSlots` and `playerOrientation` if it is the result of a login (i.e., part of the welcoming process), but that won't be the case if the message is a response to a `voir` command send, which in turn will contain a `vision` object. To handle these, we'll use the `<optional>` tools that modern C++ grants us.

`Message` just needs a header file, as it is just comprised of data and a couple of one-line query methods that are defined in the header itself. It looks like this in my implementation:
```cpp
#pragma once

#include <vector>
#include <string>
#include <optional>

// Orientation is always 0-indexed matching the server enum. N=0,E=1,S=2,W=3. Never convert this.
enum class Orientation {
	N = 0, E = 1, S = 2, W = 3
};

// tile info data struct
// stores info for a viewed tile: contents and position relative to viewing player
struct VisionTile {
	int							distance;	// 0 = player's own tile
	int							localX;		// negative = left, 0 = center, positive = right
	int							localY;		// always a distance value (amount of rows fwd)
	int							playerCount;
	std::vector<std::string>	items;

	bool hasItem(const std::string& item) const {
		for (auto tileItem : items) {
			if (tileItem == item) return true;
		}
		return false;
	}

	int countItem(const std::string& item) const {
		int count = 0;
		for (auto tileItem : items) {
			if (tileItem == item) count++;
		}
		return count;
	}
};

struct Inventory {
	int nourriture	= 0;
	int linemate	= 0;
	int deraumere	= 0;
	int sibur		= 0;
	int mendiane	= 0;
	int phiras		= 0;
	int thystame	= 0;
};

enum class MsgType {
	Unknown, Bienvenue, Welcome, Response, Event, Broadcast, Error
};

struct ServerMessage {
	MsgType		type = MsgType::Unknown;
	std::string	raw;

	// for response
	std::string cmd;
	std::string arg;
	std::string status;

	// for welcome
	std::optional<int>			mapWidth;
	std::optional<int>			mapHeight;
	std::optional<int>			remainingSlots;
	std::optional<Orientation>	playerOrientation;

	// for voir
	std::optional<std::vector<VisionTile>> vision;

	// for inventaire
	std::optional<Inventory> inventory;

	// for broadcast
	std::optional<std::string> messageText;
	std::optional<int> broadcastDirection; // 0 - same tile, 1-8 = octants (server told)

	bool isOk()         const { return status == "ok"; }
	bool isKo()         const { return status == "ko"; }
	bool isInProgress() const { return status == "in_progress"; }
	bool isDeath()      const { return status == "died"; }
	bool isLevelUp()    const { return status == "level_up"; }
};
```

And that's it. Nothing else needed here, at least not for now. We can move on to coding the process of feeding these data structs from server received JSON messages.

### 2.2.2 Like Duolingo but for Net Communications
Basically, we need to write a translator that passes JSON formed data into client data structs. Nothing new here, I've done this quite a lot of times, and the tool we'll use for it, `nlohmann json`, is pretty easy to handle. We only need one general `parse()` method here. We don't even need a parser class, just one namespace contained function that takes a raw, string-converted-JSON and extracts and stores its contents while handling the different variations it can come with:
```cpp
namespace MessageParser {
    ServerMessage parse(const std::string& raw);
}
```
It's not even a long or complex function, just one that has to manage divergent paths. You can check the full implementation in its [definition file](../client/srcs/protocol/MessageParser.cpp), but conceptually what this needs to do is:
1. Instantiate a `ServerMessage` object
2. Rebuild the raw string as a JSON to facilitate data extraction
3. Identify and store the message type
4. Extract data depending on the message type
    - If welcome: remaining lients, orientation, map sizes
    - If voir: vision object (tile by tile)
    - if inventory: inventory object (resource by resource)
    - If broadcast: message content

That simple. The only thing that calls for special attention is the fact that most of the data attributes in the `ServerMessage` struct are `std::optional`, so transfering data needs to go through a `.value()` access, which needs a safety pre-check of `.has_value()`. Always go through the check, is my advice, as even if you can be 100% sure about the contents of a message because you have acces (or even control) of the server codebase, it is good practice. And I don't think that out there, in R E A L  L I F E this all-knowing case will be the case, or even something manageable.

### 2.2.3 Commands, Fall In!
With our message handling in place, what we'll need next is a `Sender` that queues commands, allowing one in-flight command at a time (one of the past lessons, here's to you dead bots), with confirmed callbacks. In other words, we need a class that:
- Sends JSON-formed commands through the websocket in the server's direction (with specific cases for each command in *Zappy*)
- Has a way of storing answer-expecting commands and their callback functions (i.e., when the server responds, what needs to happen)
    - *We'll do this with a FIFO approach in mind, as that's what the server does*
- Has a way of checking if response-awaiting commands time out
- Has a way of processing a response (i.e., a server message carring out the result of a sent command)
- A cleanup cancel function

At this point, `Sender`'s header file is going to look something like this:
```cpp
#pragma once

#include "../net/WebsocketClient.hpp"
#include "Message.hpp"

#include <functional>
#include <chrono>
#include <deque>

class Sender {
	public:
		struct PendingCommand {
			std::string									cmd;
			std::chrono::steady_clock::time_point		sentAt;
			std::function<void(const ServerMessage&)>	callback;
		};

	private:
		WebsocketClient&			_ws;
		std::deque<PendingCommand>	_pending;

		// helper
		Result sendObject(const std::string& dump);

	public:
		explicit Sender(WebsocketClient& ws);
		~Sender() = default;

		Result sendLogin(const std::string& teamName, const std::string& key="SOME_KEY");

		virtual Result sendVoir();
		virtual Result sendInventaire();
		
		virtual Result sendAvance();
		virtual Result sendDroite();
		virtual Result sendGauche();

		virtual Result sendPrend(const std::string& resource);
		Result sendPose(const std::string& resource);

		Result sendBroadcast(const std::string& msg);
		
		Result sendIncantation();
		Result sendFork();
		Result sendConnectNbr();

		virtual void expect(const std::string& cmd, std::function<void(const ServerMessage&)> callback);
		void processResponse(const ServerMessage& msg);
		void checkTimeouts(int timeoutMs = 10000);

		void cancelAll();
};
```

Again, we'll go through it's logical specifics, but for a full code implementation, please go to its [cpp file](../client/srcs/protocol/Sender.cpp). My proposed `Sender` class holds a reference to the `WebsocketClient` and a deque (container chose for FIFO reasons) to store `_pending` commands. On top of that, there are:
- Command specific sender functions that build JSON objects with the needed fields to *do* what in our context means *act*, which is mainly telling the server that a `cmd` message is coming through.
    - Because of JSON being the formal protocol chosen for the project, these functions call the object sending function by passing them the string-converted pre-made JSON. This two step process might not be strictly *needed*, but it's an easy and sane way of making sure that the message sent to the server will be correctly formed JSON-wise.
- A general `sendObject()` function that sends the command string through the websocket and tracks the `Result` of the process.
- An `expect()` function that builds a `PendingCommand` object and pushes it to `_pending`, containing data related to `cmd`, `callback` and time it was sent at.
- A `processResponse()` function that looks up in `_pending` for the necessary callback function related to the command attached to the expected response, firing the function and erasing the command from the queue.
- A `checkTimeouts()` function that compares the `sentAt` values of a `_pending` command to their timeout limits, calling any timeout command's callback with a "timeout" status message, so that the server gets informed about the situation.
- A `cancelAll()` function that clears `_pending` (in case of disconnections and whatnot), whith the specific consideration of firing every pending command with a "canceled" status. This is needed so that `Behavior` can reset the `_commandInFlight` flag without deadlocking, in cases like a reconnection.

Some concrete decisions taken when writing this `Sender` derive from, once again, past lives. For example, when the command sent and expecting a response is `Incantation`, the first answer is not going to be its resolution, but an `in_progress` confirmation (as you know, this command is its own can of worms, a whole infernal process), and when receiving that specific message the command MUST NOT be erased from `_pending`. The need for firing up timed-out and cancelled commands are also in the realm of scar-tissue code.

At this point, things can (NEED TO) be tested. We have a message managing pipeline and a command sending class that also handles responses, so we need test suites for [MessageParsing](../client/tests/unit/MessageParsingTest.cpp) and [Sender](../client/tests/unit/SenderTest.cpp), and be sure that we have the following under control:
- Parser:
    - Welcoming message stores remaining slots and map info
    - Voir response with one tile stores all its contents (with special attention to "player" items)
    - Multi-tile Voir response stores all the received tiles with all its contents
    - Inventaire response correctly parses each resource type and its amount
    - Broadcast response correctly identifies and stores the message contents
- Sender:
    - JSON formatting for every command
    - Network error handling
        - Errors returned on Network Failure
    - Response processing
        - Callback firing
        - Compound key checkup in `_pending` (prend and pose)
        - Incantation response when `in_progress` keeps command in `_pending`
        - Unknown responses handling
    - Timeouts
        - Non expired commands are ignored
        - Expired commands get fired
    - Cancelations
        - Canceled commands are fired
        - `_pending` queue gets cleared
        - Canceling does not affect new enqueued command
    - Edge cases
        - Empty callback fire does not crash/throw in expect
        - Empty callback fire does not crash/throw in response processing
        - Multiple callbacks registered for same command results in firing the first matching one
        - Canceling with a callback that adds more commands

In my case, all these pass (YAY!!), so this marks the first pass to the `protocol` side of things as DONE AND TESTED. WHich means that we can move on to the `agent` front and build a first, single-agent survival focused approach so we reach a first compiling and non-dying client that works in unison with the server.

### 2.2.4 State of the Union
*Nothing else came to mind for a title with "state", it's 2 pm, I'm hungry, I've been writing for quite some time, sorry*. Moving on to the `agent` side of things, and now that we can actually talk with the server both in the net and in the protocol layers, we need to start tackling the game logic handling and the decision making landscape. And what does the agent need to know, before anything else, to start *behaving* and *deciding* and *AIing*? Some `state` structs, correct. In specific, we need 2 different data containers to hold information related to two main aspects: player and world. Think about it, the AI needs to know were it is, what it has stored in its inventory, the direction in which its looking, its level... All while crossing that information with the data in its vision (*what* is *where*), the overall dimensions of the arena, and have some tools to query the surroundings (i.e., asking the world if this or that resource is in this or that known tile, if there are players around, etc.). It might sound convoluted, but it's quite straightforward really, take a look:
```cpp
struct PlayerState {
	int			x = 0;
	int			y = 0;
	Orientation	orientation = Orientation::N;
	int			level = 1;
	Inventory	inventory;
	int			remainingSlots = 0;

	int food() const { return inventory.nourriture; }
};
```
```cpp
struct WorldState {
	PlayerState				player;
	std::vector<VisionTile>	vision;
	int						mapWidth = 0;
	int						mapHeight = 0;

	// NOTE: server does not send orientation in welcome; player starts at N by default.
	// If the server ever adds this field, the optional handles it correctly.
	void onWelcome(const ServerMessage& msg) {
		std::cout << msg.raw << std::endl;
		if (msg.playerOrientation.has_value())
			player.orientation = msg.playerOrientation.value();
		
		if (msg.remainingSlots.has_value())
			player.remainingSlots = msg.remainingSlots.value();

		if (msg.mapWidth.has_value())
			mapWidth = msg.mapWidth.value();
		
		if (msg.mapHeight.has_value())
			mapHeight = msg.mapHeight.value();
	}

	// helper queries
	bool visionHasItem(const std::string& item) const {}

	std::optional<VisionTile> nearestTileWithItem(const std::string& item) const {}

	int playersOnCurrentTile() const {}

	int countItemOnCurrentTile(const std::string& item) const {}
};
```
> *`WorldState` helpers are actually defined in the header file, I just didn't want to clutter this too much. Check the [file](../client/srcs/agent/State.hpp) if you want to check the implementations out*

That's really just it, at least for now (I keep writing this because I live with the fear of what's to come, I've been here, before, a few times (@TomDelongue)). What this gives is the grounds to tackle the next important class, `Behavior`, which will be slightly more difficult, but nothing we can't handle.

### 2.2.5 On Our Best Behavior
This, defined above as the client's **state machine**, is were all our headaches are going to gather around and dance to the music of their ancestors. We'll start with basic stuff, after all our current first milestone just needs to have a basic (but strong and refined and PERFECT) routine of exploring and gathering food, but I know that not much time will pass before tears come back (I feel it in the water, I feel it in the earth, I smell it in the air, Much that once w... *ehem*). What we'll do is a two step movement: **we'll first build the scaffolding of `Behavior`, then write the basis of `Navigator`, then go back to wire behaving with navigating. Sounds fun, right? Knew so.

So, yeah, behaving ourselves. Let's, again, think about what we're trying to do here from a general perspective, approach it conceptually first, code-wise then. I've been saying that this is functionally the client's state machine, so this immediately takes us to what every state machine needs: a state collection. In other words, your run of the mill `enum class` that stores every state in which the AI can operate (for now, as said, just `Idle` and `CollectFood`; the latter being self-explanatory, the former needing a definition, which for us will be the base, exploratory, take-a-walk-and-look-around state). With that, building a state machine is just a matter of *what* gets done in each state, with specific *hows*, all wrapped around logic rules on how states relate to each other and what transitions are set up.

This is not one of those things that are obvious on approach, or at least not to me, so let's go bit by bit and keep ourselves in the conceptual level for a little while longer. `Behavior` needs to be understood, in our context, as basically a loop (or, as we'll see, something *inserted* in a loop) that reads the state of the world and issues commands depending on the current objetive of the AI. For example, if the AI is in `CollectFood` state and `WorldState`'s `vision` contains, let's say, data about some food being placed in the tile currently placed one step forward and one step to the right of the client's position, `Behavior` needs to interpret said situation and plan a route to go to that position and pick up some food. Takin it into our *Zappy* logic rules, this would mean that the AI should compose a chain of commands that looks something like: *Avance→Gauche→Avance→Prend+Nourriture*. Later on this will need to be extended with more states and more possible strategical needs, like gathering specific resources, rally for incantations, fork, etc., but it wouldn't make sense for us to dive head first into those without being ABSOLUTLEY sure that the most basic tiers of our `Behavior` work without issue, which itself means that we need to be TOTALLY certain that the AI knows how to read the `WorldState`, fix an objective, find its bearings with regards of that goal and trace navigation strategies.

Now again, all of the above has to be tackled with consideration to the concept of **staleness**, which for us is something that applies to information regarding `vision` (in the world state) and `inventory` (in the player state). I've talked about this before in this log, but just in case, what I mean by this is that `Behavior` also requires tools to identify if what it *thinks* it know about the world and about itself is accurate, still true, useful to make decisions. Just go back to some past laid out possibilities, back a couple of sections in this document, and imagine a client that has planned its journey to some piece of whatever weirdly called rock and halfway there said stone disappears. Keep on keeping on could mean death, and would definately mean a waste of time and resources. Trust me, I learn this the hardes of ways, so it's better if we're absolutely clear on this before going forward. Later on, this restrain will be need to be closely paired with some self-imposed rules, like there only being once ommand in flight at a time at max, a food emergency overriding all states except `Incantating`, and who knows what more (whatever makes this work tbh).

How are we going to handle this staleness bussiness, then? Well, there might be more refined, millimetrically adjusted ways to do so than the one that I'm going to propose (I don't really know, shall I say nor care right now, as doing it like this is difficult enough for me in this moment of my life), but we'll just follow a very simple protocol: **moving makes the current vision stale, and picking or placing things up makes the current inventory and vision stale**. In simpler terms, when writing the command executing pipeline, when a moving or object related object is laid out, stale flags need to be risen. If you think about it, it makes total sense: when a client moves, for example, it means that both the server and the whole array of connected clients have gone through a `tick()`, changing the state of the world, so if a client staid true to a non-reliable state, disaster would ensue. To me, as in *to my code*, this means that once a client has built a **navigation plan** and executed the first command in it (which will in most cases be a movement or object related one), staleness insues by default and the navigation plan needs to be rebuilt on the spot. As I said, there might be better ways to do this, but this is what I can now execute with regards of having a lot of little dudes running around my *Zappy* with millisecond-to-millisecond reactiveness.

Anyways... We can't keep delaying the code confrontation, so let's get into it. Our `Behavior` needs to know about both the `WorldState` and the `Sender`, as it is kind of in between those, but because we're deep in OOP and we've already advanced that we'll have an `Agent` orchestrator, we'll create those in that and store references in this. We'll also need some flags to know if inventory and/or vision are stale and if there is an in-flight command, some storage for navigatin commands (which is really something related to the second of our two step movement, needed after writing `Navigation`, but no use in hiding it), a track for the navigation target and another one for the amount of exploration steps the client has executed, for the sake of injecting some variations here and there depending on the count, as some pattern-breaking maneuvers are always welcomed to avoid clients going into loops. Alongside this, some query functions and setters, an all-purpose navigation command executor and a main entry point to behavior, the `tick()`. And alltogether, we get this cute little class declaration:
```cpp
#pragma once

#include "../protocol/Message.hpp"
#include "State.hpp"
#include "../protocol/Sender.hpp"
#include "Navigator.hpp"

#include <cstdint>
#include <deque>

enum class AIState {
	Idle,
	CollectFood
};

class Behavior {
	private:
		Sender&				_sender;
		WorldState&			_state;
		bool				_commandInFlight = false;
		bool				_staleVision = true;
		bool				_staleInventory = true;
		std::deque<NavCmd>	_navPlan;
		std::string			_navTarget;
		int					_explorationStep = 0;

		static constexpr int FOOD_SAFE					= 12;
		static constexpr int FOOD_CRITICAL				= 4;

		void executeNavCmd(NavCmd cmd);

	public:
		Behavior(Sender& sender, WorldState& state);
		~Behavior() = default;
	
		void tick(int64_t nowMs);
		void onResponse(const ServerMessage& msg);

		bool hasCommandInFlight() const { return _commandInFlight; }
		bool isVisionStale()      const { return _staleVision; }
		bool isInventoryStale()   const { return _staleInventory; }

		// Mark vision stale — does NOT clear the nav plan.
		// The nav plan is cleared in the voir callback only if the target is gone,
		// or explicitly via clearNavPlan() when the situation changes.
		void setVisionStale()    { _staleVision = true; }
		void setInventoryStale() { _staleInventory = true; }
		void clearNavPlan()      { _navPlan.clear(); _navTarget.clear(); }
};
```

Implementation wise, for now there's not much to highlight. The coded behavior in `tick()` is one that makes the clients go around the game gathering food in the most basic of ways: is there food in the current tile? Pick it up. Is there not? Make a navigation plan towards the closes tile in `vision` that contains `nourriture`. Rinse and repeat. The `tick()` flow, then, goes like this:
- If there's an in-flight command, do nothing, i.e. wait for it to resolve
- If vision or inventory are stale, refresh them
- If current tile contains food, call `_sender.sendPrend("nourriture")`
- Else navigate for closest food in vision
- Else fallback to the `Navigator` function dedicated to create a base exploration step (which is the one that has `_explorationStep` count-dependent movement injection, more on that in the next section1)

At the bottom of `tick()` there should at least be one navigation command set up for firing, so it gets popped from the plan, executed, and we're done ticking. "Executed" here means, of course, sent to `executeNavCmd`, which works on `NavCommands`, which I'm going to explain in the next section, but this is not a complicated function, nor one that needs too much attention beyond making sure that things work *correctly*. If `NavCmd` is go forward, well, move forward depending on the current orientation, and same goes for turning left or right. The real sauce is in the `Navigation`, so let's jump into that.

### 2.2.6 Sir, this is a Wendys
We're still bound to our milestone, so we'll aim to a simple first `Navigator` scheme. We already have a `Behavior` that can trigger a navigation request, but we now need the other side of the equation, the part that builds said navigation, i.e. the navigation command chain. In practicallity, and considering an example context like wanting to go to a located food position, `Behavior` will tell `Navigator` something like *"Hey, I want to go to this X-Y position, so give me a path there considering my current orientation!"* and paths will be found and objectives will be met and we will be happy. Easier said than done, but 0 units of panic, because, you see, we have a *system* now, we now how to tackle stuff and think about stuff and decide about stuff and ultimately code about stuff. We're that good. And all that `Navigator` needs to be for now, really, is a handfull of collected functions inside a namespace. We just need ways to:
- Translate local coordinates to world deltas
- Turn to face a target
- Plan path
- Inject exploration steps

That's all. We're fine. The specific code in these? Ah, yes, well, let's see... The first two are just a couple of functions that take some data and return relative information. `turnToFace()` is pretty simple, just a couple of cases to know if going from a current `Orientation` to a target one means turning right, left, or a 180 degrees turn (i.e., two left or right turns). `localToWorldDelta()` is simple too, but needs some mind wrap around, or at least I needed it, so, just in case, let's talk about it.

First, why do we need this? Easy answer: **we're working with two coordinate systems**. When `voir` is casted, the server returns a vision grid relative to the current position and facing direction. Well, not really a *grid* but a *cone*, but that's irrelevant to the point, which is that server delivers this information:
```
You are facing NORTH (↑):
    Row 2:  [ -2,2 ] [ -1,2 ] [ 0,2 ] [ 1,2 ] [ 2,2 ]
    Row 1:           [ -1,1 ] [ 0,1 ] [ 1,1 ]
    Row 0:                    [ 0,0 ]
                     ← LEFT     YOU    RIGHT →
```

> *We've gone through it, but just in case: (0,0) is the current tile, a negative localX value means to the left, a positive localX means to the right, the value of localY is how far in front*

> *This means that, when facing north, a local tile (-1,1) is one tile ahead and to the left, and one local tile (2,2) is two tiles ahea an to the right*

All fine and dandy, but **the server has a fixed map (wrapping around the edges, but still), and treats world coordinates as absolute positions on this map**. So, while a little dude is working in its little world of local coordinates, going to a little tile that's (2, -3) from him, out there, in the game world, this might mean that said little dude is really trying to go from tile (5,8) to (7, 5) (can you guess which direction our little dude is facing???). So we need a translator. But that's tricky, because **the same local coordinates mean different world movements depending on the way our little dude is facing**. Take a look at this table realted to a simple example of our guy trying to move to the local (1,0) tile:

| Current Facing | (1,0) Meaning     | World Delta |
| -------------- | ----------------- | ----------- |
|     NORTH      | Tile to the right |    (+1,0)   |
|     EAST       | Tile to the right |    (0,+1)   |
|     SOUTH      | Tile to the right |    (-1,0)   |
|     WEST       | Tile to the right |    (0,-1)   |

> *If you find this table somewhat confusing, just try to cross-reference a general consideration of the world as absolute X-Y values, with imagining yourself in a tile, facing some direction, and what "tile to the right" would mean by watching yourself from the general pov. For example, youre facing EAST and want to go to the tile to your right. What does that look in general POV? Moving one tile down, i.e. incrementing the Y value of the X-Y coordinates by 1. Hope this helps*

If this clicks, then the rest is just laying out a conversion switch case. The values of the movements are irrelevant, the conversions are just `Orientation` based. A little dude moving NORTH will always get a `worldX` equal to its `localX`, and a `worldY` equal to the negative conversion of its `localY`. Bare in mind that our little dudes only get `voir` information (the cone from above) relative to what's *in front* of them, so conversion is thankfully constricted to that limitation. Therefore, a small, finite table of conversions can be written just like this:
```cpp
std::pair<int, int> Navigator::localToWorldDelta(Orientation facing, int localX, int localY) {
    int worldX, worldY;

    switch (facing) {
        case Orientation::N:  // Facing NORTH
            worldX = localX;           // Your right is world +X
            worldY = -localY;          // Your forward is world -Y
            break;

        case Orientation::E:  // Facing EAST
            worldX = localY;           // Your forward is world +X
            worldY = localX;           // Your right is world +Y
            break;

        case Orientation::S:  // Facing SOUTH
            worldX = -localX;          // Your right is world -X
            worldY = localY;           // Your forward is world +Y
            break;

        case Orientation::W:  // Facing WEST
            worldX = -localY;          // Your forward is world -X
            worldY = -localX;          // Your right is world -Y
            break;
    }

    return {worldX, worldY};
}
```
Told you, easy to say, not so easy to write. You just need to spend some time spatially thinking about what you're trying to translate, some experimentation, maybe even some doodling, and you're set. You can now translate the client's navigation coordinates into server absolute coordinates. And with both the functions to face to a specific orientation and to translate coordinates, we're ready to go into path planning. And we'll do so with a specific strategy: **first move laterally (X), then forward (Y)**. Why? Because we have to do it some way, and this works. And this, taken to `planPath()`, means:
- If `localX` is not 0, little needs to move laterally, so we'll `localToWorldDelta()` the coordinates, check how it needs to turn via `turnToFace()`, then go `Forward` the needed amount of times.
	- *Basically, we need to plan an "L" movement.*
- Once `localX` is 0, the little dude needs to turn back to face the target `localY` direction relative to its `Orientation`
- Then, walk Y amount of times `Forward`

All of this is made into an `std::vector` of `NavCmd`s, which are stored in `enum class NavCmd { Forward, TurnLeft, TurnRight };`. And we're done!! Nothing else needs to be done at this point, in our current milestone, besides a very basic `explorationStep()` function that just injects left and right turns when `_explorationSteps` are divisible by 13 or 7. Why, again? Because this works: enough, but not too much variation every 100 steps. But you do you, of course!

> *Down the line, if the decision is made, pathfinding could be reformulated to follow an `A*` algorithm, but we'll see. I've already implemented this in [another project of mine](https://github.com/hugomgris/rosario-engine/blob/main/srcs/AI/Pathfinder.cpp), so I don't really feel the irk, but* Dios lo ve *so who knows, who can tell..*

After hooking things up in `Behavior` so that it works with `NavCmds`, we're ready to write `main`, but first there's a lot of testing to be done. Once more, you can check out the suite's code in their respective files, [BehaviorTest](../client/tests/unit/BehaviorTest.cpp) and [NavigatorTest](../client/tests/unit/NavigatorTest.cpp), so we'll just list here what needs to be tested:
- Behavior
	- Stale vision and inventory reactions
	- In-flight command blocks tick
	- Vision and Inventory updates are data-correct
	- Target relative positions trigger correct command chain (i.e., target food to the left queues a gauche, and so on)
	- Moving makes vision stale
	- No target sends a droite for exploration (i.e., the little dude turns to get a different cone of vision)
- Navigator
	- Coordinate translation correctness
	- Planned path is correctly sequenced (for the target "L" shaped movement)
	- Exploration step correctly injects the intended variations.

And off we are to the realm of `Main`... Wait, no, we're still in need of the whole wrapper of the client. Change of plans: off we are to write `Agent`!

### 2.2.7 Agent Little Dude
As this is the general wrapping class, we're going to place in it all the functions related to general stuff, like those related to the connection process (`connect()`, `waitForBienvenue()` and `performLogin()`), a couple of start and stop functions and the core of it all: `networkLoop()` + `procesIncomingMessage()`. The first tier, the network related stuff, is uninteresting, just a close-following of the needed protocol, imposed by the server. `run()` sends the client off to its own thread and sets `_running` as true, and `stop()` sets it as false, just before attempting to join the client thread and calling `Sender`'s `cancelAll()`.

The `networkLoop()`, our bot's beating heart (GO YOU, CHARLES THE BOT THE THIRD!!) is uncomplicated:
- Update the value of `nowMs`
- `tick()` the `WebsocketClient`
- Call the incoming message processor
- Manage the reconnection attempts in case of a disconnect
- Call `Sender`'s timeout managing function
- `tick()` the `Behavior`
- Sleep for some miliseconds
- REPEAT

As the logic of everything listed above is delegated to the respective classes and namespaces and these have already been logged, no need to go into further detail, right? Well, I guess we should talk about `processIncomingMessages()`, but this is really a function that reads the received `WebSocketFrame`, checks the `ServerMessage` contents and derives the flow to the necessary path of the codebase. Just go to its [implementation](../client/srcs/agent/Agent.cpp) if you need more details, as we're now moving on to our last step before having one full milestone in our back!

> *I'll test Agent thoroughly down the line, don't fret*


### 2.2.8 Is It Alive?
For our first `main.cpp`, we need the following:
- Parse the CLI arguments (via an already tested and solid `Parser`)
- Set up signal handlers
- Create the `Agent`, call it's `connect()` function, then `run()` it and embrace the loop

We're basically set up for testing, which in my case means:
- Firing up the server
- Resuming its time API (this is just a way to syncronize server, clients and observer via a SIGUSR1 signal)
- Connect the client

> *I've set up a [script](../run_test.sh) to quick-fire the probe test, because doing this manually is grinding me*

If everything is done properly, our first little dude should thrive, survive, live a long, infinite level-1 life of plentifulness and amass `nourriture` like there was no tomorrow. 

And... That's the case for me!! So If you excuse me, I'll spend the rest of my day (evening) basking in this tiny success and do something, whatever, not related to being sitting in front of a computer.

Next time, adding complexity to our little dude! See you in the 3rd devlog!