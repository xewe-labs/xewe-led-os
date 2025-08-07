#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include <algorithm>
#include <cctype>

// Returns a new string where every character in the input is converted to lowercase.
inline std::string lower(std::string s) {
    std::transform(
        s.begin(), s.end(),
        s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
    );
    return s;
}

#endif // STRING_UTILS_HPP
