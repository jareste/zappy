#include "agent/Agent.hpp"
#include "helpers/Logger.hpp"
#include "helpers/Parser.hpp"
#include "../incs/Result.hpp"

#include <iostream>
#include <csignal>
#include <cstring>
#include <getopt.h>

static Agent* g_agent = nullptr;
static volatile sig_atomic_t g_stop_requested = 0;

void signalHandler(int sig) {
	if (sig == SIGINT || sig == SIGTERM)
		g_stop_requested = 1;
}

void printUsage(const char* progName) {
	std::cout << "Usage: " << progName << " [options] <host> <port> <team_name>\n"
			<< "Options:\n"
			<< "  --no-fork         Disable automatic forking (default: enabled)\n"
			<< "  --key				Specify server key value\n"
			<< "  --debug           Enable debug logging\n"
			<< "  --help            Show this help\n";
}

int main(int argc, char **argv) {
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN); // to reap zombie processes (fork related)

	bool forkEnabled = true;
	bool debugMode = false;

	static struct option long_options[] = {
		{"no-fork", no_argument, 0, 'F'},
		{"debug",   no_argument, 0, 'd'},
		{"key",     required_argument, 0, 'k'},
		{"help",    no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};


	std::string serverKey = "SOME_KEY"; 

	int opt;
	while ((opt = getopt_long(argc, argv, "Fdk:h", long_options, nullptr)) != -1) {
		switch (opt) {
			case 'F': forkEnabled = false; break;
			case 'd': debugMode = true; break;
			case 'k': serverKey = optarg; break;
			case 'h': printUsage(argv[0]); return 0;
			default:  printUsage(argv[0]); return 1;
		}
	}

	if (optind + 3 > argc) {
		std::cerr << "Error: Missing required arguments" << std::endl;
		printUsage(argv[0]);
		return 1;
	}

	std::string host = argv[optind];
	int port = std::stoi(argv[optind + 1]);
	std::string teamName = argv[optind + 2];

	Logger::setLevel(debugMode ? LogLevel::Debug : LogLevel::Info);

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	Logger::info("Zappy Client starting...");
	Logger::info("Team: " + teamName);
	Logger::info("Fork enabled: " + std::string(forkEnabled ? "yes" : "no"));

	Agent agent(host, port, teamName, serverKey);
	g_agent = &agent;
	agent.setForkEnabled(forkEnabled);

	// EASY MODE CHECK
	const char* easyModeEnv = std::getenv("ZAPPY_EASY_ASCENSION");
	bool easyMode = (easyModeEnv && std::string(easyModeEnv) == "1");
	agent.setEasyMode(easyMode);
	if (easyMode) {
		Logger::info("Easy ascension mode enabled (ZAPPY_EASY_ASCENSION=1)");
	} else
		Logger::info("Normal mode enabled");

	Result res = agent.connect(Agent::CONNECT_TIMEOUT_MS);
	if (!res.ok()) {
		Logger::error("Failed to connect: " + res.message);
		return 1;
	}

	Logger::info("Connected! Starting main loop...");

	res = agent.run();
	if (!res.ok()) {
		Logger::error("Client error: " + res.message);
		return 1;
	}

	while (agent.isRunning()) {
		if (g_stop_requested) {
			std::cout << std::endl << "Signal received. Shutting down..." << std::endl;
			agent.stop();
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	Logger::info("Client exited. Final level: " +
				std::to_string(agent.getState().player.level));

	return 0;
}