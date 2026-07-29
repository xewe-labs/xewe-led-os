// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Tests/Nvs/NvsTester.cpp
#include "NvsTester.h"

#include "../../Modules/Module/ModuleController.h"


NvsTester::NvsTester(ModuleController& controller)
      : Module(controller,
               /* id                  */ "nvs_test",
               /* name                */ "NvsTester",
               /* description         */ "Round-trip and isolation tests for the Nvs module",
               /* requires_init_setup */ false,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ true) {

    commands_storage.push_back(Command{
        "run",
        "Run the full Nvs test suite",
        std::string("$") + id + " run",
        0,
        [this](std::span<const std::string>) { run_tests(); }
    });

    commands_storage.push_back(Command{
        "clean",
        "Wipe all Nvs test namespaces",
        std::string("$") + id + " clean",
        0,
        [this](std::span<const std::string>) {
            clean();
            this->controller.serial_port.print("Test namespaces cleared");
        }
    });
}


void NvsTester::report(const char* label, bool ok) {
    if (ok) {
        m_pass++;
        controller.serial_port.printf("  [PASS] %s", label);
    } else {
        m_fail++;
        controller.serial_port.printf("  [FAIL] %s", label);
    }
}


template <typename T>
void NvsTester::round_trip(const char* label,
                           std::string_view ns,
                           std::string_view key,
                           const T& value) {
    controller.nvs.write<T>(ns, key, value);
    const T got = controller.nvs.read<T>(ns, key);
    report(label, got == value);
}


template <typename WriteT>
void NvsTester::str_round_trip(const char* label,
                               std::string_view ns,
                               std::string_view key,
                               const WriteT& value,
                               const char* expected) {
    controller.nvs.write<WriteT>(ns, key, value);
    const std::string got = controller.nvs.read<std::string>(ns, key);
    report(label, got == expected);
}


void NvsTester::clean() {
    controller.nvs.reset_ns(kNs);
    controller.nvs.reset_ns(kNs2);
    controller.nvs.reset_ns(kNsBtn);
}


void NvsTester::run_tests() {
    m_pass = 0;
    m_fail = 0;

    controller.serial_port.print_header("Nvs Test Suite");

    // Start from a known-clean slate.
    clean();

    // ---- string-like write types (all read back as std::string) -----------
    str_round_trip<std::string>     ("str_std",     kNs, "s_std",  std::string("hello world"), "hello world");
    str_round_trip<String>          ("str_arduino", kNs, "s_ard",  String("hello world"),      "hello world");
    str_round_trip<std::string_view>("str_view",    kNs, "s_view", std::string_view("hello world"), "hello world");
    str_round_trip<const char*>     ("str_cstr",    kNs, "s_cstr", "hello world",              "hello world");

    // Empty string and a string that fully uses the 15-char key budget.
    str_round_trip<std::string>     ("str_empty",   kNs, "s_empty", std::string(""), "");
    str_round_trip<std::string>     ("str_maxkey",  kNs, "maxkeylen_12345", std::string("k"), "k");

    // std::string read back as Arduino String (the other supported read type).
    {
        controller.nvs.write<std::string>(kNs, "s_asstr", std::string("hello world"));
        const String got = controller.nvs.read<String>(kNs, "s_asstr");
        report("str_read_as_String", got == "hello world");
    }

    // A key longer than 15 chars must be REJECTED, not silently truncated
    // (truncation would collide keys sharing a 15-char prefix). write() returns
    // false and the value never lands, so the read comes back as the default.
    // (Expect a "Nvs: ERROR name ... too long" line printed twice here.)
    {
        const char* long_key = "key_hello_dear_robert_adams"; // 27 chars
        const bool wrote = controller.nvs.write<int32_t>(kNs, long_key, 123);
        const int32_t got = controller.nvs.read<int32_t>(kNs, long_key, -1);
        report("overlength_key_rejected", !wrote && got == -1);
    }

    // ---- scalar types -----------------------------------------------------
    round_trip<bool>       ("bool_true",  kNs, "b_true",  true);
    round_trip<bool>       ("bool_false", kNs, "b_false", false);
    round_trip<char>       ("char",       kNs, "c_char",  static_cast<char>('Z'));
    round_trip<int8_t>     ("i8",         kNs, "i8",      static_cast<int8_t>(-42));
    round_trip<uint8_t>    ("u8",         kNs, "u8",      static_cast<uint8_t>(200));
    round_trip<int16_t>    ("i16",        kNs, "i16",     static_cast<int16_t>(-1234));
    round_trip<uint16_t>   ("u16",        kNs, "u16",     static_cast<uint16_t>(60000));
    round_trip<int32_t>    ("i32",        kNs, "i32",     static_cast<int32_t>(-100000));
    round_trip<uint32_t>   ("u32",        kNs, "u32",     static_cast<uint32_t>(4000000000u));
    round_trip<int64_t>    ("i64",        kNs, "i64",     static_cast<int64_t>(-5000000000LL));
    round_trip<uint64_t>   ("u64",        kNs, "u64",     static_cast<uint64_t>(10000000000ULL));
    round_trip<float>      ("float",      kNs, "flt",     3.14159f);
    round_trip<double>     ("double",     kNs, "dbl",     2.718281828459045);

    // ---- default value on a missing key -----------------------------------
    {
        const int got = controller.nvs.read<int>(kNs, "missing_key", 999);
        report("default_on_missing", got == 999);
    }

    // ---- per-namespace isolation ------------------------------------------
    // Same key name in two namespaces must not collide.
    {
        controller.nvs.write<int32_t>(kNs,  "shared", 1);
        controller.nvs.write<int32_t>(kNs2, "shared", 2);
        const int32_t a = controller.nvs.read<int32_t>(kNs,  "shared");
        const int32_t b = controller.nvs.read<int32_t>(kNs2, "shared");
        report("ns_isolation", a == 1 && b == 2);
    }

    // ---- Buttons collision regression -------------------------------------
    // Under the old composite-key scheme, "buttons:btn_cfg_0/1/10" all truncated
    // to the same 15-char slot and clobbered each other. These must stay distinct.
    {
        controller.nvs.write<int32_t>(kNsBtn, "btn_cfg_0",  100);
        controller.nvs.write<int32_t>(kNsBtn, "btn_cfg_1",  101);
        controller.nvs.write<int32_t>(kNsBtn, "btn_cfg_10", 110);
        const int32_t c0  = controller.nvs.read<int32_t>(kNsBtn, "btn_cfg_0");
        const int32_t c1  = controller.nvs.read<int32_t>(kNsBtn, "btn_cfg_1");
        const int32_t c10 = controller.nvs.read<int32_t>(kNsBtn, "btn_cfg_10");
        report("buttons_no_collision", c0 == 100 && c1 == 101 && c10 == 110);
    }

    // ---- remove() ---------------------------------------------------------
    {
        controller.nvs.write<int32_t>(kNs, "to_remove", 7);
        controller.nvs.remove(kNs, "to_remove");
        const int32_t got = controller.nvs.read<int32_t>(kNs, "to_remove", -1);
        report("remove", got == -1);
    }

    // ---- reset_ns() -------------------------------------------------------
    {
        controller.nvs.write<int32_t>(kNs2, "k1", 11);
        controller.nvs.write<int32_t>(kNs2, "k2", 22);
        controller.nvs.reset_ns(kNs2);
        const int32_t k1 = controller.nvs.read<int32_t>(kNs2, "k1", -1);
        const int32_t k2 = controller.nvs.read<int32_t>(kNs2, "k2", -1);
        report("reset_ns", k1 == -1 && k2 == -1);
    }

    // ---- persistence across reads (reopen handle each time) ---------------
    // Distinct read after other writes confirms the value is committed to flash,
    // not just cached in an open handle.
    {
        controller.nvs.write<std::string>(kNs, "persist", std::string("kept"));
        controller.nvs.write<int32_t>(kNs, "noise", 999);
        const std::string got = controller.nvs.read<std::string>(kNs, "persist");
        report("persist_after_commit", got == "kept");
    }

    // Leave no test data behind.
    clean();

    controller.serial_port.printf("Nvs tests done: %d passed, %d failed", m_pass, m_fail);
}
