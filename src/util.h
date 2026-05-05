#pragma once

#include <string>

std::string trim_copy(const std::string& input);
std::string to_lower_copy(const std::string& input);

bool try_parse_double(const std::string& text, double& value);
bool is_iso8601_utc_loose(const std::string& text);
