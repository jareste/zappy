#include "Navigator.hpp"
#include "../helpers/Logger.hpp"

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

		default:
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

		Orientation xFacing;
		if      (dx ==  1 && dy ==  0) xFacing = Orientation::E;
		else if (dx == -1 && dy ==  0) xFacing = Orientation::W;
		else if (dx ==  0 && dy == -1) xFacing = Orientation::N;
		else                           xFacing = Orientation::S;
		
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

std::vector<NavCmd> Navigator::planApproachDirection(int broadcastDirection, Orientation) {
	std::vector<NavCmd> commands;
	
	Logger::info("DIRECTION RESULT = " + std::to_string(broadcastDirection));
	
	switch (broadcastDirection) {
		case 1: case 2: case 8:
			break;
			
		case 3: case 4:
			commands.push_back(NavCmd::TurnRight);
			break;
			
		case 5:
			commands.push_back(NavCmd::TurnRight);
			commands.push_back(NavCmd::TurnRight);
			break;
			
		case 6: case 7:
			commands.push_back(NavCmd::TurnLeft);
			break;
	}
	
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
