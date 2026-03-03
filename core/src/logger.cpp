#include <pixelferrite/logger.hpp>
#include <pixelferrite/colors.hpp>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace pixelferrite {

namespace {

void write_line(const char* level, std::string_view message) {
    colors::initialize();

    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &timestamp);
#else
    localtime_r(&timestamp, &local_time);
#endif

    std::string_view level_color = colors::info();
    if (std::string_view(level) == "ERROR") {
        level_color = colors::error();
    }

    std::ostringstream stamp;
    stamp << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    std::clog << colors::wrap(stamp.str(), colors::dim())
              << " [" << colors::wrap(level, level_color) << "] "
              << message << '\n';
}

}  // namespace

void Logger::info(std::string_view message) {
    write_line("INFO", message);
}

void Logger::error(std::string_view message) {
    write_line("ERROR", message);
}

}  // namespace pixelferrite
