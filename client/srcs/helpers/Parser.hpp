#pragma once

#include "../../incs/Result.hpp"
#include "../../incs/DataStructs.hpp"
#include "Logger.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

class Parser {
	
	
	public:
		static Result parseArguments(char** argv, Arguments& parsedArguments);
		static Result evaluateArguments(Arguments& parsedArguments);
		static void printParsedArguments(Arguments& parsedArguments);
		static void printUsage();
};