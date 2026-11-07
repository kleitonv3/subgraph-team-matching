#include "format.hpp"

#include <algorithm>
#include <cstdio>

std::string formatThousands(std::uint64_t value) {
    const std::string digits = std::to_string(value);
    std::string result;
    const int length = static_cast<int>(digits.size());
    for (int i = 0; i < length; ++i) {
        if (i > 0 && (length - i) % 3 == 0) result += '.';
        result += digits[i];
    }
    return result;
}

std::string formatDuration(double seconds) {
    char buffer[64];
    if (seconds < 1.0) {
        std::snprintf(buffer, sizeof buffer, "%.3f ms", seconds * 1000.0);
    } else {
        std::snprintf(buffer, sizeof buffer, "%.3f s", seconds);
    }
    std::string text = buffer;
    std::replace(text.begin(), text.end(), '.', ',');
    return text;
}

std::string formatCount(std::uint64_t count, const char* singular, const char* plural) {
    return formatThousands(count) + " " + (count == 1 ? singular : plural);
}
