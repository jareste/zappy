#pragma once

#include <string>

struct Arguments {
	std::string		teamName = "";
	int				port = -1;
	std::string		hostname = "localhost";
	int				clientCount = 1;
	bool			insecure = false;
	bool			loopMode = false;
};