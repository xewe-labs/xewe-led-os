// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Tests/Nvs/NvsTester.h
#pragma once

#include "../../Modules/Module/Module.h"

#include <cstdint>
#include <string>
#include <string_view>


struct NvsTesterConfig : public ModuleConfig {};


// Test module that exercises the Nvs module end to end.
// Run from the CLI with: $nvstest run
//
// Method bodies live in NvsTester.cpp so they see the complete ModuleController
// type (they dereference controller.nvs / controller.serial_port). Here we only
// forward-declare ModuleController (via Module.h), which is enough to declare a
// ModuleController& parameter and store the reference in the Module base.
class NvsTester : public Module {
public:
    explicit NvsTester(ModuleController& controller);

private:
    // All test data lives under these namespaces so real module data is never
    // touched; every namespace is wiped before and after a run.
    static constexpr const char* kNs      = "nvstest";
    static constexpr const char* kNs2     = "nvstest2";
    static constexpr const char* kNsBtn   = "nvstest_btn";

    int m_pass = 0;
    int m_fail = 0;

    void report(const char* label, bool ok);

    template <typename T>
    void round_trip(const char* label, std::string_view ns, std::string_view key, const T& value);

    // write<WriteT> covers more string-like types (std::string, Arduino String,
    // std::string_view, const char*) than read<T> can return, so every string
    // variant is written as its own type and read back as std::string.
    template <typename WriteT>
    void str_round_trip(const char* label, std::string_view ns, std::string_view key,
                        const WriteT& value, const char* expected);

    void run_tests();
    void clean();
};
