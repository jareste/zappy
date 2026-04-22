# Fork + Connect_nbr Implementation Roadmap

## Overview

The goal of this feature is to let your AI clients self-replicate: when a client has enough food and is at a high enough level, it forks an egg, waits for it to hatch server-side, and then a new AI process connects and claims the resulting slot. This increases your team's total player count over the course of a game without relying solely on the initial slots.

The server side is fully ready (see server assessment). This document covers everything you need to build on the client side, in the order you should build it.

---

## Part 1 — Understanding the Server Contract

Before writing a single line, make sure these invariants are burned in, because they drive every design decision below.

**Fork timing.** When a client sends `fork`, the server schedules an egg to hatch after `EGG_HATCH_DELAY` time units (currently 600). The `fork` command itself responds `ok` immediately — the hatch is asynchronous. There is no "egg ready" notification pushed to the client. You have to either wait a known duration or poll with `connect_nbr`.

**connect_nbr semantics.** The server returns `max_players - (current_players + p2c_pending)`. `p2c_pending` only increments when the egg *hatches*, not when `fork` is sent. So in the window between `fork ok` and hatch, `connect_nbr` still returns 0 (or whatever it was before). This is the key timing trap: do not poll `connect_nbr` immediately after a fork.

**Claim flow.** When a new process connects with the team name, `game_register_player` checks for a pending `p2c` entry. If one exists, the new connection is grafted onto that player slot (inheriting position, inventory, and food). If no `p2c` entry exists (e.g. you connected too early, or the egg died), registration falls through to a normal new-player spawn — which may fail if the team is at capacity.

**One egg per fork.** Each `fork` command creates exactly one egg. If you want N extra players you need N forks, spaced out.

**Egg death.** Eggs are not immortal. An unclaimed egg whose player runs out of food will be cleaned up. The hatched player starts with `nourriture = 10` and a fresh `die_time`, so the window is `TIME_TO_DIE` time units from hatch — but the new process needs to connect before that expires.

---

## Part 2 — New State and Fields

### 2.1 New AIState values

Add to `AIState` in `Behavior.hpp`:

```cpp
enum class AIState {
    // ... existing values ...
    Forking,        // 8  — waiting for fork ok, then waiting for hatch
    WaitingForHatch // 9  — hatch delay has been started, polling connect_nbr
};
```

`Forking` covers the period from "decision to fork" through `fork ok`. `WaitingForHatch` covers the period from `fork ok` through `connect_nbr > 0`.

### 2.2 New fields in Behavior.hpp (private section)

```cpp
// Fork / connect_nbr
bool        _forkSent             = false;
int64_t     _forkSentMs           = 0;      // wall time when fork ok was received
int64_t     _hatchPollIntervalMs  = 2000;   // how often to poll connect_nbr
int64_t     _lastHatchPollMs      = 0;
int64_t     _hatchTimeoutMs       = 0;      // absolute deadline for hatch claim
int         _pendingEggCount      = 0;      // eggs we've forked and not yet seen claimed
bool        _connectNbrInFlight   = false;
```

### 2.3 Constants

Add near the existing `FOOD_FORK` constant:

```cpp
static constexpr int     FORK_MIN_LEVEL        = 2;    // don't fork at level 1
static constexpr int     HATCH_DELAY_UNITS      = 600;  // must match server EGG_HATCH_DELAY
static constexpr int     UNITS_PER_MS           = ???;  // derive from your time_api calibration
static constexpr int64_t HATCH_POLL_START_MS    = 4000; // don't poll before this many ms post-fork
static constexpr int64_t HATCH_TIMEOUT_MS       = 30000;// give up waiting after 30 s
static constexpr int     MAX_PENDING_EGGS        = 2;   // cap concurrent unhatched eggs
```

`UNITS_PER_MS` is the relationship between server time units and real milliseconds. You need this to convert `HATCH_DELAY_UNITS` into a safe real-time poll delay. If your time_api exposes the frequency, compute it there. If not, 600 units at a typical `t = 100` means 6 seconds — use 6000 ms as `HATCH_POLL_START_MS` conservatively.

---

## Part 3 — Fork Decision Logic

### 3.1 Where the fork decision lives

The fork decision currently lives inline in `tickCollectStones` with a simple food threshold check. That check triggers correctly but it does not account for:

- Whether this client is already in a leadership/rally flow (forking during a rally is wasteful and risks food starvation).
- Whether there are already unclaimed eggs pending (forking again before the first egg is claimed wastes food).
- Whether the team actually has capacity for another player (forking when the team is full is pointless — the egg hatches but the slot can't be claimed if the team is at hard cap).

### 3.2 Refactor the fork decision into a helper

Remove the inline fork block from `tickCollectStones` and replace it with a call to a new method:

```cpp
bool Behavior::shouldFork() const {
    if (_state.player.level < FORK_MIN_LEVEL)           return false;
    if (_state.player.food() < FOOD_FORK)               return false;
    if (!_state.forkEnabled)                            return false;
    if (_pendingEggCount >= MAX_PENDING_EGGS)           return false;
    if (_forkInProgress || _forkSent)                   return false;

    // Don't fork while in any rally-related state — we need food for the rally.
    if (_aiState == AIState::Leading       ||
        _aiState == AIState::ClaimingLeader||
        _aiState == AIState::MovingToRally ||
        _aiState == AIState::Rallying      ||
        _aiState == AIState::Incantating)
        return false;

    return true;
}
```

Then in `tickCollectStones`, replace the existing fork block with:

```cpp
if (shouldFork()) {
    _aiState = AIState::Forking;
    clearNavPlan();
    return;
}
```

### 3.3 tickForking()

```cpp
void Behavior::tickForking() {
    if (_forkSent) return; // waiting for fork ok

    _forkSent        = true;
    _commandInFlight = true;

    Logger::info("Behavior: sending fork at level " +
        std::to_string(_state.player.level) +
        " food=" + std::to_string(_state.player.food()));

    _sender.sendFork();
    _sender.expect("fork", [this](const ServerMessage& msg) {
        _commandInFlight = false;
        _forkSent        = false;

        if (msg.isOk()) {
            Logger::info("Behavior: fork OK — egg laid, entering WaitingForHatch");
            _forkSentMs       = _lastTickMs;
            _lastHatchPollMs  = _lastTickMs;
            _hatchTimeoutMs   = _lastTickMs + HATCH_TIMEOUT_MS;
            _pendingEggCount++;
            _forkInProgress   = true;
            _aiState          = AIState::WaitingForHatch;
        } else {
            Logger::warn("Behavior: fork KO — back to CollectStones");
            _forkInProgress = false;
            _aiState        = AIState::CollectStones;
        }
    });
}
```

Add `tickForking()` to the switch in `tick()`:

```cpp
case AIState::Forking:        tickForking();           break;
case AIState::WaitingForHatch: tickWaitingForHatch(nowMs); break;
```

---

## Part 4 — Hatch Polling Logic

### 4.1 The timing problem in detail

You cannot use a fixed sleep. The server runs at a configurable `t` (frequency), and your client has no direct knowledge of when exactly 600 server time units have elapsed. What you can do is:

1. Start polling `connect_nbr` only after `HATCH_POLL_START_MS` real milliseconds have passed since `fork ok` (giving the egg time to hatch).
2. Poll every `_hatchPollIntervalMs` thereafter.
3. When `connect_nbr` returns > 0, the egg has hatched and a slot is available.
4. If `_hatchTimeoutMs` elapses without a positive `connect_nbr`, give up and return to normal operation.

### 4.2 tickWaitingForHatch()

```cpp
void Behavior::tickWaitingForHatch(int64_t nowMs) {
    // Keep eating if food is getting low — egg will wait.
    if (_state.player.food() < FOOD_CRITICAL) {
        Logger::warn("Behavior: WaitingForHatch — food critical, abandoning wait");
        _forkInProgress  = false;
        _pendingEggCount = std::max(0, _pendingEggCount - 1);
        _aiState         = AIState::CollectFood;
        return;
    }

    // Absolute timeout — the egg may have died or the hatch was missed.
    if (nowMs >= _hatchTimeoutMs) {
        Logger::warn("Behavior: WaitingForHatch timed out — egg may have died");
        _forkInProgress  = false;
        _pendingEggCount = std::max(0, _pendingEggCount - 1);
        _aiState         = AIState::CollectStones;
        return;
    }

    // Don't even start polling until enough real time has passed.
    if (nowMs - _forkSentMs < HATCH_POLL_START_MS)
        return;

    // Rate-limit the polls.
    if (nowMs - _lastHatchPollMs < _hatchPollIntervalMs)
        return;

    _lastHatchPollMs    = nowMs;
    _connectNbrInFlight = true;
    _commandInFlight    = true;

    _sender.sendConnectNbr();
    _sender.expect("connect_nbr", [this](const ServerMessage& msg) {
        _commandInFlight    = false;
        _connectNbrInFlight = false;

        if (!msg.connectNbr.has_value()) {
            Logger::warn("Behavior: connect_nbr response malformed");
            return;
        }

        int slots = msg.connectNbr.value();
        Logger::info("Behavior: connect_nbr = " + std::to_string(slots));

        if (slots > 0) {
            Logger::info("Behavior: egg hatched! Slot available — spawning child process");
            _pendingEggCount = std::max(0, _pendingEggCount - 1);
            _forkInProgress  = false;
            spawnChildClient();
            _aiState = AIState::CollectStones;
        }
        // If slots == 0, keep waiting — egg hasn't hatched yet.
    });
}
```

---

## Part 5 — Child Process Spawning

### 5.1 Overview

When `connect_nbr` returns > 0, you need to launch a new AI client process that will connect to the server, send the team name, and be grafted onto the hatched egg slot. There are two valid approaches:

**Approach A — `fork()` + `exec()`** (recommended): The current process `fork()`s a child, which `exec()`s a fresh copy of your AI binary with the same server address, port, and team name as arguments. The child runs as a completely independent process with its own state.

**Approach B — In-process thread**: Spawn a new thread that opens a second connection. Simpler to implement but means one crashed thread can affect the parent, and the `Sender`/`WorldState`/`Behavior` objects must be fully thread-safe (they currently aren't).

Use Approach A. It matches how the initial clients are launched and requires no threading changes.

### 5.2 spawnChildClient()

This method needs access to the server address, port, and team name. The cleanest way is to store them in `WorldState` or pass them in through the constructor. Assuming they live in `_state`:

```cpp
void Behavior::spawnChildClient() {
    Logger::info("Behavior: spawning child AI process for team " + _teamName);

    pid_t pid = fork();

    if (pid < 0) {
        Logger::error("Behavior: fork() syscall failed: " + std::string(strerror(errno)));
        return;
    }

    if (pid == 0) {
        // Child process — exec a new instance of the AI binary.
        // Construct the same argument vector the parent was launched with,
        // or build it from stored config.
        std::string host    = _state.serverHost;
        std::string port    = std::to_string(_state.serverPort);
        std::string team    = _teamName;

        // Replace the child process image with a fresh AI client.
        // Adjust the binary path to match your build output.
        execl("./ai_client",
              "./ai_client",
              "-h", host.c_str(),
              "-p", port.c_str(),
              "-n", team.c_str(),
              nullptr);

        // execl only returns on error.
        Logger::error("Behavior: execl failed: " + std::string(strerror(errno)));
        _exit(1);
    }

    // Parent process — child is running independently. No need to wait().
    // If you want to track children, store the pid somewhere.
    Logger::info("Behavior: child AI process spawned, pid=" + std::to_string(pid));
}
```

Add `#include <unistd.h>` and `#include <cerrno>` and `#include <cstring>` to `Behavior.cpp` if not already present.

### 5.3 Storing server connection info

Add to `WorldState` (or wherever your connection config lives):

```cpp
std::string serverHost;
int         serverPort = 0;
```

Populate these when the initial connection is established, before constructing `Behavior`. Pass them in through the `Behavior` constructor or directly into `WorldState` — whichever is cleaner with your existing architecture.

---

## Part 6 — Sender Changes

You need one new method on `Sender`:

```cpp
void Sender::sendConnectNbr() {
    sendJson({{"type", "cmd"}, {"cmd", "connect_nbr"}});
}
```

And the expect key should be `"connect_nbr"`. The server response format is:

```json
{"type":"response","cmd":"connect_nbr","arg":"3"}
```

The slot count is in the `arg` field as a string. In your `ServerMessage` parsing layer, add:

```cpp
if (cmd == "connect_nbr" && argNode) {
    msg.connectNbr = std::stoi(argNode->valuestring);
}
```

Add `std::optional<int> connectNbr` to `ServerMessage` in `Message.hpp`.

---

## Part 7 — Zombie Process Handling

When you `fork()` without `wait()`ing, zombie processes accumulate in the process table until the parent exits. For a long game this is a real problem. Two clean solutions:

**Option A — SIGCHLD handler**: In your main or network init, add:

```cpp
signal(SIGCHLD, SIG_IGN);
```

On Linux, setting `SIGCHLD` to `SIG_IGN` causes the kernel to automatically reap children without creating zombies. This is the simplest fix.

**Option B — Non-blocking waitpid in tick()**: In `tick()`, periodically call:

```cpp
while (waitpid(-1, nullptr, WNOHANG) > 0) {}
```

This reaps any children that have finished without blocking. Add it as a housekeeping call at the top of `tick()` every N ticks if you prefer explicit control.

Use Option A unless you need to track child exit codes.

---

## Part 8 — Integration with Existing tickCollectStones

The current `tickCollectStones` has an inline fork block that you should remove as part of step 3.2:

```cpp
// REMOVE THIS ENTIRE BLOCK:
if (_state.player.food() > FOOD_FORK && _state.player.level >= 2 && _state.forkEnabled) {
    Logger::info("Fork call triggered");
    _aiState = AIState::CollectStones;
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

Replace with the single `shouldFork()` call from section 3.2. The old block ignores the `fork ok`/`ko` distinction, doesn't track pending eggs, and goes straight back to `CollectStones` without waiting for a child to connect — which means the egg hatches but no one claims it.

---

## Part 9 — Edge Cases and Failure Modes

### 9.1 Fork during food shortage

If the parent forks and then immediately falls below `FOOD_CRITICAL` while in `WaitingForHatch`, the code in section 4.2 correctly abandons the wait and switches to `CollectFood`. The egg will remain alive on the server for `TIME_TO_DIE` units. If the parent recovers food in time, it won't re-poll (it already exited `WaitingForHatch`) — the egg will eventually die unclaimed. This is an acceptable loss; don't try to resume polling after recovery, as the complexity isn't worth it.

### 9.2 Multiple forks

`_pendingEggCount` caps concurrent unhatched eggs at `MAX_PENDING_EGGS`. Do not raise this above 2 unless your team has a large food surplus — each `WaitingForHatch` period locks the parent into a low-activity state.

### 9.3 connect_nbr returns > 1

If `connect_nbr` returns 2 or more, it means either you have multiple hatched eggs pending or another team member's egg also hatched. Only spawn one child per positive `connect_nbr` response and decrement `_pendingEggCount` by 1. The remaining slots will be visible on the next poll or will be claimed by other clients.

### 9.4 The child connects too fast

If the child `exec()`s and connects before `p2c_pending` is incremented server-side (i.e. before hatch), `game_register_player` will treat it as a new player rather than a claim. Given your `HATCH_POLL_START_MS` guard this shouldn't happen, but if you ever see children spawning at level 1 with no inventory instead of inheriting the egg's state, this is why — add more delay before the child connects, or have the child itself poll `connect_nbr` before sending login.

### 9.5 execl path

The `execl` path `"./ai_client"` assumes the binary is in the current working directory when the server runs. Harden this by using `argv[0]` from `main()` stored at startup, or resolve the absolute path with `realpath()`.

---

## Part 10 — Implementation Order

Work through this in order. Each step is independently testable before moving to the next.

**Step 1** — Add `connectNbr` to `ServerMessage`, add `sendConnectNbr()` to `Sender`, verify that a manual `connect_nbr` command returns the right JSON and is parsed correctly.

**Step 2** — Add the new fields to `Behavior.hpp`, add the two new `AIState` values, wire them into the `tick()` switch. Stub `tickForking()` and `tickWaitingForHatch()` with just a log + fallback to `CollectStones`.

**Step 3** — Implement `shouldFork()` and `tickForking()`. Remove the old inline fork block from `tickCollectStones`. Verify a client reaches `WaitingForHatch` and then falls back after timeout (no child spawning yet).

**Step 4** — Implement `tickWaitingForHatch()` with real `connect_nbr` polling. Verify that after a fork, the client polls, sees `connect_nbr > 0` when the egg hatches, and logs "egg hatched" before falling back to `CollectStones`. Still no child spawning.

**Step 5** — Implement `spawnChildClient()`. Add `serverHost`/`serverPort` to `WorldState`. Add `SIGCHLD` suppression in `main()`. Test that the child process connects, receives the `welcome` message with the inherited position, and behaves as a normal client from there.

**Step 6** — Full integration test: let a game run long enough for multiple fork cycles. Confirm `_pendingEggCount` stays bounded, no zombie processes accumulate, and the team's player count grows over time.

---

## Summary of All Files to Touch

| File | Change |
|---|---|
| `Behavior.hpp` | Add `AIState::Forking`, `AIState::WaitingForHatch`; add 7 new private fields |
| `Behavior.cpp` | Add `shouldFork()`, `tickForking()`, `tickWaitingForHatch()`, `spawnChildClient()`; refactor `tickCollectStones()`; add `#include <unistd.h>` |
| `Message.hpp` | Add `std::optional<int> connectNbr` to `ServerMessage` |
| `Sender.hpp/cpp` | Add `sendConnectNbr()` |
| `State.hpp` | Add `serverHost`, `serverPort` to `WorldState` |
| `main.cpp` (or network init) | Add `signal(SIGCHLD, SIG_IGN)` |
| Message parsing layer | Parse `connect_nbr` arg into `msg.connectNbr` |
