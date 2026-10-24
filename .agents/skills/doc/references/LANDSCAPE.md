# Competing tools

A survey of the ESP32 ecosystem, current on 2026-10-11. Read it before adding a feature that an existing tool may already do, or before claiming something is unique.

## What is unique and what is not

| Capability | Closest existing tools | Verdict |
|---|---|---|
| `$group command args` CLI | arduino-esp32 3.3 `Console`, ESP32SerialCtl, AdvancedCLI | partial: grouping exists, per-module ownership does not |
| Uniform `status`, `enable`, `disable`, `reset` verbs | Tasmota naming convention | unique but small |
| Typed NVS settings | Arduino `Preferences`, MycilaConfig (MIT) | partial: per-module namespaces and strict mismatch errors are new |
| Settings schema queryable at runtime (`$<id> schema`) | none | unique, and the gap that would help agents most |
| Bounded prompts with timeout and retries | Shellminator (timeouts only) | unique, and contrarian: product firmwares avoid prompts |
| `XeWeOs` facade and module contract as an includable library | ESPHome, WLED usermods, Tasmota drivers | unique, the core differentiator |
| Wi-Fi, web, time, scheduler, buttons, pins modules | WiFiManager, MycilaESPConnect, ESP32-sveltekit | covered, so wrap rather than reimplement |
| Pinning in `xewe.toml` | arduino-cli `sketch.yaml` profiles, IDF `dependencies.lock` | mostly covered |
| Flash that keeps NVS | arduino-cli upload | covered |
| `xewe serial` | arduino-cli, pio, idf.py monitors | covered |
| `xewe test` board runner | pytest-embedded | mostly covered |
| Compile-only fallback without a board (D22) | manual CI wiring | unique, easy to copy |
| Dotenv first-boot provisioning (`xewe provision`) | Improv Serial | unique, should interoperate with Improv |
| Agent-oriented development (D23) | ESPHome, WLED, Tasmota agent guides, Meshtastic MCP server | covered, and behind Meshtastic |

## Facts that decide work

- arduino-cli profiles (1.3 and later) cannot pin git refs, ZIP URLs, content hashes, `--build-property` values or tool versions other than the platform's, and need one profile per board. Those limits are the only reasons `xewe.toml` exists beside a profile.
- pytest-embedded (MIT) has an `arduino` service with diff flashing, port selection by serial number, MAC or USB location, several boards and Wokwi, QEMU and esp-emu backends. It lacks the compile-only fallback, a wait for native-USB re-enumeration with the boot banner, and a provisioning step. Those three are the reason `xewe test` is its own runner.
- The `.merged.bin` is padded to full flash size and wipes NVS when written at 0x0. arduino-esp32's normal upload writes only bootloader, partition table, boot_app0 and app.
- Improv Serial carries Wi-Fi credentials, device info and the device name, but no timezone or custom settings, and has no maintained Python host.
- Tasmota, Meshtastic and OpenMQTTGateway are GPL-3.0 and WLED is EUPL-1.2, so their module code cannot be lifted into a permissive library. This template and the modules are GPL-3.0-only, so the permissive-library gap is only partly XeWe's to claim. The license of the core and modules is open.
- ESPHome tests are compile-only and its includable library `esphome-core` was archived in 2021. Homie and Basecamp have had no release since 2020.
- An unbounded prompt in XeWe firmware would be a regression. Prompts are defensible because they are bounded and `xewe provision` answers them.

## Candidates, not decisions

1. Generate one `sketch.yaml` profile per chip from `xewe.toml` and let arduino-cli install and cache. Keep `xewe.toml` for what profiles cannot pin. `arduino-cli --dump-profile` bootstraps one.
2. Build `xewe test` on pytest-embedded's `arduino` and `serial-esp` services and keep the compile-only fallback, the re-enumeration wait and the provisioning fixture.
3. Evaluate `xewe::Cli` on arduino-esp32 `Console`, keeping `$group` dispatch, the uniform verbs and paste-safe queueing as a layer above it. It gains line editing, history and HWCDC and costs the IDF console's REPL task model.
4. Wrap MycilaESPConnect or WiFiManager in the wifi module and keep ArduinoJson under `xewe::FlexData`.
5. Speak Improv Serial for the Wi-Fi and device-name part of first boot, so ESP Web Tools hosts work. The dotenv helper keeps timezone and module settings.
6. A device-wide command that dumps every module's commands and settings schema as JSON. `$<id> schema` exists per module, the device-wide manifest does not.
7. Later: move the ESP Web Tools flasher into the org, fed by the Release asset URLs, and prototype web UIs under constraints that port to a web-interface module.

## Not covered by the survey

The PlatformIO registry and the full Arduino index were not searched for NVS-schema or JSON-data-layer libraries, so a niche equivalent may exist. It is unconfirmed whether pytest-embedded reopens a port after USB re-enumeration. Serial pretty-printers were not assessed. The absence of another `setup.sh` and `run.sh` template bundle is a negative search result.
