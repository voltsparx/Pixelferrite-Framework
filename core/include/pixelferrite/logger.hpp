#pragma once

#include <string_view>

namespace pixelferrite {

class Logger {
public:
    static void info(std::string_view message);
    static void error(std::string_view message);
};

}  // namespace pixelferrite
