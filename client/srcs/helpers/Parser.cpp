#include "Parser.hpp"


using ErrorCode = ErrorCode;

Result Parser::parseArguments(char** argv, Arguments& parsedArguments) {
	std::vector<std::string> args;
	for (int i = 1; argv[i]; ++i) {
		args.push_back(std::string(argv[i]));
	}

	if (args.size() < 4) {
		return Result::failure(ErrorCode::InvalidArgs, "Error: Invalid arguments");
	}

	for (size_t i = 0; i < args.size(); ++i) {
		const std::string& arg = args[i];
		
		if (arg == "--insecure") {
			parsedArguments.insecure = true;
			continue;
		}
		
		if (arg == "--loop") {
			parsedArguments.loopMode = true;
			continue;
		}
		
		if (arg == "-n" || arg == "-p" || arg == "-h" || arg == "-c") {
			if (i + 1 >= args.size()) {
				return Result::failure(ErrorCode::InvalidArgs, "Error: Missing value for argument: " + arg);
			}
			const std::string& value = args[i + 1];
			if (arg == "-n") {
				parsedArguments.teamName = value;
			} else if (arg == "-p") {
				parsedArguments.port = std::stoi(value);
			} else if (arg == "-h") {
				parsedArguments.hostname = value;
			} else if (arg == "-c") {
				parsedArguments.clientCount = std::stoi(value);
			}
			++i;
			continue;
		}

		std::cout << "failing with: " << arg << std::endl;

		return Result::failure(ErrorCode::InvalidArgs, "Error: Unrecognized argument: " + arg);
	}

	return Result::success();
}

Result Parser::evaluateArguments(Arguments& parsedArguments) {
	if (parsedArguments.teamName.empty()) {
		return Result::failure(ErrorCode::MissingArgs, "Error: Missing required argument: team name");
	} else if (parsedArguments.port == -1) {
		return Result::failure(ErrorCode::MissingArgs, "Error: Missing required argument: port");
	}
	
	return Result::success();
}

void Parser::printParsedArguments(Arguments& parsedArguments) {
	Logger::debug("Parsed arguments:");
	Logger::debug("teamname: " + parsedArguments.teamName);
	Logger::debug("port: " + std::to_string(parsedArguments.port));
	Logger::debug("hostname: " + parsedArguments.hostname);
	Logger::debug("client count: " + std::to_string(parsedArguments.clientCount));
	Logger::debug(std::string("insecure: ") + (parsedArguments.insecure ? "true" : "false"));
	Logger::debug(std::string("loop mode: ") + (parsedArguments.loopMode ? "true" : "false"));
}

void Parser::printUsage() {
	std::cout << "Usage: ./client -n <team> -p <port> [-h <hostname>] [-c <count>] [--insecure] [--loop]" << std::endl
				<< "-n <teamName>   : Name of the team (required)" << std::endl
				<< "-p <port>        : Port number (required)" << std::endl
				<< "-h <hostname>    : Hostname (default: localhost)" << std::endl
				<< "-c <client_cnt>  : Number of clients to run (default: 1)" << std::endl
				<< "--insecure       : Disable TLS cert verification (test only)" << std::endl
				<< "--loop           : Keep connected and send periodic voir commands" << std::endl;
}