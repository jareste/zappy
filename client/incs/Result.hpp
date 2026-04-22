#pragma once

#include <string>

enum class ErrorCode {
	Ok = 0,
	InvalidArgs,
	MissingArgs,
	IoError,
	NetworkError,
	ProtocolError,
	InternalError
};

struct Result {
	ErrorCode code;
	std::string message;

	[[nodiscard]] bool ok() const { return code == ErrorCode::Ok; }

	static Result success() { return {ErrorCode::Ok, ""}; }

	static Result failure(ErrorCode c, std::string msg) {
		return {c, std::move(msg)};
	}
};