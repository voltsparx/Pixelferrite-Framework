#pragma once

#include <string>
#include <string_view>

namespace pixelferrite::colors {

void initialize();
bool enabled();

std::string wrap(std::string_view text, std::string_view ansi_code);

std::string_view reset();
std::string_view dim();
std::string_view bold();

std::string_view brand();
std::string_view accent();
std::string_view orange();
std::string_view gray();
std::string_view info();
std::string_view success();
std::string_view warning();
std::string_view error();
std::string_view prompt();

std::string_view payload();
std::string_view exploit();
std::string_view auxiliary();
std::string_view encoder();
std::string_view evasion();
std::string_view nop();
std::string_view transport();
std::string_view detection();
std::string_view analysis();
std::string_view lab();

std::string_view for_category(std::string_view category);

}  // namespace pixelferrite::colors
