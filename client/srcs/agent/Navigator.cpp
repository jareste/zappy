#include "Navigator.hpp"
#include "../helpers/Logger.hpp"

// Orientation is always 0-indexed matching the server enum: N=0,E=1,S=2,W=3. Never convert this
// NEVER CONVERT THIS
// I emphasize this to myself because converting this was a mayor pitfall in past builds
std::pair<int, int> Navigator::localToWorldDelta(Orientation facing, int localX, int localY) {
	int worldX, worldY;

	switch (facing) {
		default:
		case Orientation::N:
			worldX =  localX;
			worldY = -localY;
			break;

		case Orientation::E:
			worldX =  localY;
			worldY =  localX;
			break;

		case Orientation::S:
			worldX = -localX;
			worldY =  localY;
			break;

		case Orientation::W:
			worldX = -localY;
			worldY = -localX;
			break;
	}

	return { worldX, worldY };
}

std::vector<NavCmd> Navigator::turnToFace(Orientation current, Orientation target) {
	int diff = (static_cast<int>(target) - static_cast<int>(current) + 4) % 4;

	std::vector<NavCmd> turns;
	switch (diff) {
		case 1:
			turns.push_back(NavCmd::TurnRight);
			break;

		case 2:
			turns.push_back(NavCmd::TurnRight);
			turns.push_back(NavCmd::TurnRight);
			break;

		case 3:
			turns.push_back(NavCmd::TurnLeft);
			break;

		default: // diff == 0: already facing target, no turns needed
			break;
	}

	return turns;
}

// TODO: OPTIONAL: Move to an A* approach
std::vector<NavCmd> Navigator::planPath(Orientation facing, int localX, int localY) {
	std::vector<NavCmd> commands;

	if (localX == 0 && localY == 0)
		return commands;

	if (localX != 0) {
		auto [dx, dy] = localToWorldDelta(facing, 1, 0);

		// dx/dy will be one of (1,0),(−1,0),(0,1),(0,−1) — map to Orientation
		Orientation xFacing;
		if      (dx ==  1 && dy ==  0) xFacing = Orientation::E;
		else if (dx == -1 && dy ==  0) xFacing = Orientation::W;
		else if (dx ==  0 && dy == -1) xFacing = Orientation::N;
		else                           xFacing = Orientation::S; // dx==0, dy==1
		
		if (localX < 0) {
			xFacing = static_cast<Orientation>((static_cast<int>(xFacing) + 2) % 4);
		}

		auto turnCmds = turnToFace(facing, xFacing);
		commands.insert(commands.end(), turnCmds.begin(), turnCmds.end());

		for (int i = 0; i < std::abs(localX); ++i)
			commands.push_back(NavCmd::Forward);

		auto returnCmds = turnToFace(xFacing, facing);
		commands.insert(commands.end(), returnCmds.begin(), returnCmds.end());
	}

	for (int i = 0; i < localY; ++i)
		commands.push_back(NavCmd::Forward);

	return commands;
}

std::vector<NavCmd> Navigator::explorationStep(int& stepCount) {
	std::vector<NavCmd> commands;
	stepCount++;

	if (stepCount % 13 == 0)
		commands.push_back(NavCmd::TurnLeft);
	else if (stepCount % 7 == 0 || stepCount == 1)
		commands.push_back(NavCmd::TurnRight);

	commands.push_back(NavCmd::Forward);
	return commands;
}

// Maps a broadcast direction (1-8, 0=same tile) to a sequence of turns + one Forward.
//
// The server encodes direction as a clock-face sector relative to the *listener's*
// facing direction, where 1 = straight ahead and the numbers increase clockwise:
//
//        1  (forward)
//      8   2
//    7       3
//      6   4
//        5  (behind)
//
// We map these 8 sectors to 4 quadrants (offsets from current facing):
//   offset 0  = forward    → turn 0
//   offset 1  = right      → turn right once
//   offset 2  = behind     → turn right twice
//   offset 3  = left       → turn left once
//
// Mapping (from roadmap spec):
//   dir 1      → offset 0  (dead ahead)
//   dir 2, 8   → offset 0  (forward-ish, slight right/left — still go forward)
//   dir 3, 4   → offset 1  (right)
//   dir 5      → offset 2  (behind)
//   dir 6, 7   → offset 3  (left)
//
// Returns turn commands followed by one NavCmd::Forward.
// Returns just NavCmd::Forward if direction is 0 or out of range.
std::vector<NavCmd> Navigator::planApproachDirection(int broadcastDirection, Orientation /*currentFacing*/) {
	std::vector<NavCmd> commands;
	
	Logger::info("DIRECTION RESULT = " + std::to_string(broadcastDirection));
	
	switch (broadcastDirection) {
		case 1: case 2: case 8:
			break;
			
		case 3: case 4:
			// Right quadrant - turn right once
			commands.push_back(NavCmd::TurnRight);
			break;
			
		case 5:
			// Behind - turn around (two rights or two lefts)
			commands.push_back(NavCmd::TurnRight);
			commands.push_back(NavCmd::TurnRight);
			break;
			
		case 6: case 7:
			// Left quadrant - turn left once
			commands.push_back(NavCmd::TurnLeft);
			break;
	}
	
	// Always move forward after turning (or if already facing forward)
	commands.push_back(NavCmd::Forward);
	
	return commands;
}

/*
In case of A*

struct PathNode {
	int x, y;
	std::vector<NavCmd> path;
};

std::vector<NavCmd> findPathToTile(Orientation facing, int startLocalX, int startLocalY, int targetLocalX, int targetLocalY);
*/
