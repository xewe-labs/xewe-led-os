// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Tests/NvsFlex/NvsFlexTester.h
#pragma once

#include "../../Modules/Module/Module.h"

#include <string_view>


struct NvsFlexTesterConfig : public ModuleConfig {};


// Test module that exercises the Nvs typed-object API (nvs.save / nvs.load) and
// the FlexData codec end to end. Run from the CLI with: $flex_test run
//
// Method bodies live in NvsFlexTester.cpp so they see the complete
// ModuleController type (they dereference controller.nvs / controller.serial_port).
// Here we only forward-declare ModuleController (via Module.h). Test structs are
// defined at file scope in the .cpp.
class NvsFlexTester : public Module {
public:
    explicit NvsFlexTester(ModuleController& controller);

private:
    // All test data lives under this namespace so real module data is never
    // touched; it is wiped before and after a run.
    static constexpr const char* kNs = "flex_test";

    int m_pass = 0;
    int m_fail = 0;

    void report(const char* label, bool ok);
    void clean();
    void run_tests();
};
