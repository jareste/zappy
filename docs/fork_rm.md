Fork pipeline roadmap
Here's a pragmatic order of implementation:
Phase 1 — Controlled forking (no connect_nbr yet)
The fork trigger already exists in tickCollectStones gated on FOOD_FORK. The problem is it forks unconditionally whenever food is high enough, with no awareness of how many players are already alive. Before implementing connect_nbr, add a simple fork budget: track a _forkCount that increments on each fork, and cap it (e.g., max 2 forks per client lifetime). This prevents fork spam on a generous server.
Phase 2 — connect_nbr awareness
connect_nbr tells you how many slots remain on the team. A slot being available means a forked egg is waiting. The logic should be:

On startup, query connect_nbr.
Only fork if connect_nbr < TARGET_SLOTS (where TARGET_SLOTS is something like 3–5 free slots you want to maintain).
Re-query connect_nbr periodically (every 10s or so), not on every tick.

Phase 3 — New client spawning
This is external to the client itself — it requires your launcher/main to listen for available eggs and spawn new client processes. The typical approach is: a "nurse" process queries connect_nbr, and when it's above 0, it forks a new client process. This is simpler than trying to do it from inside an existing client.
Phase 4 — Fork coordination
In competitive conditions you don't want every client forking. Add a fork_leader broadcast analogous to claim_leader, so only one client forks at a time per level bracket. This is low priority for the probe but important for not wasting food in competition.
The ordering matters because Phase 1 and 2 can be done entirely within Behavior.cpp, while Phase 3 touches your launcher and is a separate concern.