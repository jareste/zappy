#include <gtest/gtest.h>
#include "../../srcs/helpers/Parser.hpp"
#include "../../incs/DataStructs.hpp"
#include "../../srcs/helpers/Logger.hpp"

TEST(Parser, ArgumentsAreCorrectlyParsedAndValidated_WhenGoodArgumentsFull) {
	const char *inputArguments[11] = {"Client", "-n", "alpha", "-p", "8674", "-h", "localhost", "-c", "1", "--insecure"};
	inputArguments[sizeof(inputArguments)] = 0;

	Arguments parsedArguments = {};
	Result parseResult = Parser::parseArguments(const_cast<char**>(inputArguments), parsedArguments);

	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::Ok));

	EXPECT_EQ(parsedArguments.teamName, "alpha");
	EXPECT_EQ(parsedArguments.port, 8674);
	EXPECT_EQ(parsedArguments.hostname, "localhost");
	EXPECT_EQ(parsedArguments.clientCount, 1);
	EXPECT_TRUE(parsedArguments.insecure);

	parseResult = Parser::evaluateArguments(parsedArguments);
	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::Ok));
}

TEST(Parser, ArgumentsAreCorrectlyParsedAndValidated_WhenGoodArgumentsPartial) {
	const char *inputArguments[11] = {"Client", "-n", "alpha", "-p", "8674", "--insecure"};
	inputArguments[sizeof(inputArguments)] = 0;

	Arguments parsedArguments = {};
	Result parseResult = Parser::parseArguments(const_cast<char**>(inputArguments), parsedArguments);

	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::Ok));

	EXPECT_EQ(parsedArguments.teamName, "alpha");
	EXPECT_EQ(parsedArguments.port, 8674);
	EXPECT_EQ(parsedArguments.hostname, "localhost");
	EXPECT_EQ(parsedArguments.clientCount, 1);
	EXPECT_TRUE(parsedArguments.insecure);

	parseResult = Parser::evaluateArguments(parsedArguments);
	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::Ok));
}

TEST(Parser, RejectsInputArguments_WhenBadFlag) {
	const char *inputArguments[12] = {"Client", "-a", "alpha", "-p", "8674", "-h", "localhost", "-c", "1", "--insecure", "true"};
	inputArguments[sizeof(inputArguments)] = 0;

	Arguments parsedArguments = {};
	Result parseResult = Parser::parseArguments(const_cast<char**>(inputArguments), parsedArguments);

	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::InvalidArgs));
}

TEST(Parser, RejectsInputArguments_WhenMissingArgument) {
	const char *inputArguments[8] = {"Client", "-a", "alpha", "-h", "localhost", "-c", "1"};
	inputArguments[sizeof(inputArguments)] = 0;

	Arguments parsedArguments = {};
	Result parseResult = Parser::parseArguments(const_cast<char**>(inputArguments), parsedArguments);

	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::InvalidArgs));
}

TEST(Parser, RejectsInputArguments_WhenTooFewArguments) {
	const char *inputArguments[8] = {"Client", "-h", "localhost", "-c", "1"};
	inputArguments[sizeof(inputArguments)] = 0;

	Arguments parsedArguments = {};
	Result parseResult = Parser::parseArguments(const_cast<char**>(inputArguments), parsedArguments);
	parseResult = Parser::evaluateArguments(parsedArguments);

	EXPECT_EQ(static_cast<int>(parseResult.code), static_cast<int>(ErrorCode::MissingArgs));
}