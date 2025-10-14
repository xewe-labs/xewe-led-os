#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint> // add this if not already included

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

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

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

inline std::string center_text(std::string text,
                               uint16_t total_width,
                               const std::string& edge_characters = "|")
{
    const size_t total = static_cast<size_t>(total_width);
    const size_t edge_len = edge_characters.size();

    // Degenerate cases
    if (total == 0) return std::string{};
    if (edge_len * 2 >= total) {
        // Not enough room for inner content; return as much of the left edge as fits
        return edge_characters.substr(0, total);
    }

    const size_t inner_width = total - (edge_len * 2);

    // Truncate text if needed
    if (text.size() > inner_width) {
        text.resize(inner_width);
    }

    // Compute left/right padding
    const size_t spaces = inner_width - text.size();
    const size_t left_pad  = spaces / 2;
    const size_t right_pad = spaces - left_pad;

    return edge_characters
         + std::string(left_pad, ' ')
         + text
         + std::string(right_pad, ' ')
         + edge_characters;
}

inline std::string generate_split_line(uint16_t total_width,
                                       char major_character = '-',
                                       const std::string& edge_characters = "|")
{
    const size_t total = static_cast<size_t>(total_width);
    const size_t edge_len = edge_characters.size();

    if (total == 0) return std::string{};
    if (edge_len == 0) {
        // No edges: whole line is the major character
        return std::string(total, major_character);
    }
    if (edge_len * 2 >= total) {
        // Not enough room for inner content; return as much of the left edge as fits
        return edge_characters.substr(0, total);
    }

    const size_t inner_width = total - (edge_len * 2);
    return edge_characters + std::string(inner_width, major_character) + edge_characters;
}

inline std::string capitalize(std::string s) {
    bool new_word = true;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (std::isalnum(c)) {
            s[i] = static_cast<char>(new_word ? std::toupper(c) : std::tolower(c));
            new_word = false;
        } else {
            new_word = true; // next alnum starts a new word
        }
    }
    return s;
}


} // namespace xewe::str



#endif // STRING_UTILS_HPP
