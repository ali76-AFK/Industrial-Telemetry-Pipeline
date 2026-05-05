#include "util.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

std::string trim_copy(const std::string& input) {
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

std::string to_lower_copy(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool try_parse_double(const std::string& text, double& value) {
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);

    if (end == text.c_str()) {
        return false;
    }

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }

    return true;
}

bool is_iso8601_utc_loose(const std::string& text) {
    if (text.size() != 19) {
        return false;
    }

    return std::isdigit(text[0]) &&
           std::isdigit(text[1]) &&
           std::isdigit(text[2]) &&
           std::isdigit(text[3]) &&
           text[4] == '-' &&
           std::isdigit(text[5]) &&
           std::isdigit(text[6]) &&
           text[7] == '-' &&
           std::isdigit(text[8]) &&
           std::isdigit(text[9]) &&
           (text[10] == 'T' || text[10] == ' ') &&
           std::isdigit(text[11]) &&
           std::isdigit(text[12]) &&
           text[13] == ':' &&
           std::isdigit(text[14]) &&
           std::isdigit(text[15]) &&
           text[16] == ':' &&
           std::isdigit(text[17]) &&
           std::isdigit(text[18]);
}
