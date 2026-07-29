// src/Utils/Validator.h
#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <type_traits>
#include <stdexcept>

namespace xewe {

    template <typename>
    struct always_false : std::false_type {};

    // Use LimitT to allow implicit type deduction for literals (e.g. 0, 255)
    template <typename T, typename LimitT>
    std::optional<T> validate(std::string_view value, LimitT min, LimitT max) {
        using U = typename std::decay<T>::type;

        if constexpr (std::is_same_v<U, std::string>) {
            // For strings, min and max denote the string length
            const std::size_t len = value.length();
            if (len >= static_cast<std::size_t>(min) && len <= static_cast<std::size_t>(max)) {
                return std::string(value);
            }
        }
        else if constexpr (std::is_integral_v<U> && !std::is_same_v<U, bool>) {
            // For integers, min and max denote the numeric range bounds
            try {
                const std::string s(value);
                if constexpr (std::is_signed_v<U>) {
                    long long res = std::stoll(s);
                    if (res >= static_cast<long long>(min) && res <= static_cast<long long>(max)) {
                        return static_cast<T>(res);
                    }
                } else if constexpr (std::is_unsigned_v<U>) {
                    unsigned long long res = std::stoull(s);
                    if (res >= static_cast<unsigned long long>(min) && res <= static_cast<unsigned long long>(max)) {
                        return static_cast<T>(res);
                    }
                }
            } catch (...) {
                // Catch std::invalid_argument or std::out_of_range
            }
        }
        else if constexpr (std::is_floating_point_v<U>) {
            // For floats, min and max denote the numeric range bounds
            try {
                const std::string s(value);
                double res = std::stod(s);
                if (res >= static_cast<double>(min) && res <= static_cast<double>(max)) {
                    return static_cast<T>(res);
                }
            } catch (...) {
            }
        }
        else {
            static_assert(always_false<U>::value, "Unsupported Validator::validate<T>() type.");
        }

        return std::nullopt;
    }

} // namespace xewe