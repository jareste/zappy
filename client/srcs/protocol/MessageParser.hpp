#pragma once
#include "Message.hpp"

namespace MessageParser {
    ServerMessage parse(const std::string& raw);
}