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

#pragma once
#include <string>
#include <algorithm>
#include <cctype>

namespace xewe::str {

// Replace all occurrences of `from` with `to` in s
inline void replace_all(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// Trim spaces and lowercase
inline std::string lc(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(),
            [](unsigned char c){ return std::isspace(c); }), s.end());
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

} // namespace xewe::str


#endif // STRING_UTILS_HPP
