<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# HomeAssistant: Home Assistant over MQTT auto-discovery

## Overview

Project-local module of xewe-led-os (`src/HomeAssistant/`). It puts the LED into Home Assistant
through an MQTT broker (the Home Assistant Mosquitto add-on works) with MQTT auto-discovery: one
light (on/off, brightness, hue/saturation), a Mode select and one number entity per parameter of
the running mode. Topics, unique ids and payloads are those of xewe-led-os 2.3, so an existing
installation and the HACS integration keep their entities. What changed since 2.3:
`doc/UPGRADING_FROM_2_3.md`.

| File | Contents |
|---|---|
| `HomeAssistant.h/.cpp` | the module: lifecycle, first-boot wait, connection, `/provision`, `/deprovision`, NVS |
| `HaBridge.h` | everything published over MQTT and every MQTT command applied to the LED |
| `HaDiscovery.h` | topic names, discovery and state payloads, colour conversions (standard library only) |

| | |
|---|---|
| id (CLI group, NVS namespace) | `homeassistant` |
| name | `Home_Assistant` |
| setup on first boot | yes: asks to enable, then waits until a broker is provisioned and MQTT connects |
| can be disabled | yes |
| requires | `wifi`, `web_interface`, `led` |
| declared in the sketch | `HomeAssistant homeassistant(os, wifi, web_interface, led);` after the generated modules and before `homekit` |

## How it works

### First boot (blocking)

On the boot where the module is first enabled, and again on the boot after `$homeassistant reset`
or `disable` + `enable` (both wipe its namespace), the module:

1. loads the stored broker and connects when there is one;
2. when there is none, advertises the device over mDNS: host `xewe-led-os-<last 6 hex of the
   MAC>`, service `_xewe-led-os._tcp` on port 80, TXT `mac=<12 hex>`, `fw=<firmware version>`,
   `name=<device name>`;
3. prints the provisioning instructions (the module does not start the HTTP server: the web
   interface has already started it, on every boot including its own setup boot);
4. loops, serving HTTP and printing a status line every 3 s, until the broker is provisioned
   (HACS integration or `POST /provision`) and MQTT has connected and published discovery. A line
   starting with `x` disables the module and restarts. mDNS is stopped as soon as a broker is
   provisioned, on `x`, and after the loop; it never runs on later boots.

`xewe provision` cannot drive this wait: it needs a broker or `x`.

### HTTP (on the web interface server, every boot while enabled)

| Route | Body | Answer |
|---|---|---|
| `POST /provision` | `{"host":"<mqtt-host>","port":1883,"user":"<user>","pass":"<pass>"}`; missing fields default to `""` / 1883 | invalid JSON: `400 {"error":"bad_json"}`. Otherwise the broker is stored and a connection attempted first, then `200 {"ok":true}`. An empty host stores "unprovisioned" and also answers `200 {"ok":true}` |
| `POST /deprovision` | ignored | `200 {"ok":true}`, then the retained entities are cleared, `offline` is published, MQTT disconnects and the broker is forgotten. The module stays enabled and idles |

Other methods on these paths reach the server's catch-all (the web interface's, or Alexa's when
enabled). No CORS headers on the two routes.

### MQTT

`<dev>` = `xewe_led_os_<12 hex of the MAC>`, `<base>` = `xewe_led_os/<dev>`.

| Topic | Direction | Payload |
|---|---|---|
| `<base>/avail` | out, retained, last will | `online` / `offline` |
| `<base>/light/set` | in | `{"state":"ON"\|"OFF","brightness":0-255,"color":{"h":0-360,"s":0-100}}`, applied in the order state, brightness, colour; a missing `h` or `s` keeps the current value |
| `<base>/light/state` | out, retained | `{"state":"ON","brightness":128,"color_mode":"hs","color":{"h":120,"s":100}}` |
| `<base>/mode/set` | in | a mode name (as in the Mode select) |
| `<base>/mode/state` | out, retained | the current mode name |
| `<base>/param/<key>/set` | in | a decimal value; the led clamps it; the result is published back |
| `<base>/param/<key>/state` | out, retained | decimal value |
| `homeassistant/light/<dev>/config` | out, retained | light discovery |
| `homeassistant/select/<dev>/config` | out, retained | Mode select discovery (options in led order) |
| `homeassistant/number/<dev>/<key>/config` | out, retained | one per parameter of the running mode; the previous mode's are cleared |

The device object of every entity carries `"manufacturer":"XeWe","model":"LED"` and
`"sw_version":"<firmware version>"`. Every message is retained. Exact payloads: `HaDiscovery.h`,
`HaBridge.h` and `tests/unit/test_home_assistant.py`.

The client uses a 2048-byte buffer, the device id as client id, the stored user/password (none
when empty), and reconnects at most every 3 s while Wi-Fi is up. A connect attempt blocks for the
TCP/MQTT timeouts.

### Listeners

The module is a led listener, registered in `begin_routines_required` (so a disabled module never
receives a change). Every LED change, whatever its source, is published at once from inside the
callback: colour, brightness, state -> light state; mode -> mode state, the mode's number
entities, light state; `$led reset` -> state, brightness, colour, then mode. Its own commands are
not filtered out: the echo is how Home Assistant gets the confirmed state.

### Commands

Only the generic ones: `$homeassistant status | reset | enable | disable`.

`status` prints, while enabled:

```
Home Assistant: not provisioned (POST /provision to pair)        (or)
Home Assistant: broker <host>:<port> [connected|disconnected]
Home_Assistant module enabled
```

While disabled it prints nothing (`$system status` shows `Home Assistant module disabled`).

`reset` clears the retained entities, publishes `offline`, disconnects, forgets the broker and
restarts; the next boot runs the first-boot wait again.

### NVS keys (namespace `homeassistant`)

`host` (string), `port` (u16, default 1883), `user` (string), `pass` (string, stored as text), plus
the core's `is_enabled`, `not_first_boot`, `init_complete`.

## Known issues

- Discovery advertises port 80 and the instructions print `http://<device-ip>/provision`: HACS
  discovery needs the web interface on port 80.
- The broker password is stored in plain text in NVS.
- `xewe provision` cannot answer the first-boot wait.
- A second `/provision` while connected stores the new broker (status shows it) but the module
  stays connected to the old one until that connection drops.
- A connect attempt blocks the main loop for the TCP/MQTT timeouts, every 3 s while the broker is
  unreachable (HomeKit, Alexa and the web pages pause meanwhile).

## Tests

- `tests/unit/test_home_assistant.py` with `tests/unit/host_ha_discovery.cpp`: topics, ids and
  payloads as exact 2.3 strings, a comparison with the 2.3 ArduinoJson builders (light states:
  every hue, saturation and brightness byte; the whole 655k-state grid with
  `XEWE_TEST_HA_FULL_GRID=1`), `HaBridge.h` run on the host with a fake MQTT client and led (every
  publish with its payload and retain flag, every command), and every function of
  `HomeAssistant.cpp` pinned whole.
- `tests/board/test_home_assistant.py`: status and `$system status` order, HTTP provisioning
  (`XEWE_TEST_HA_HTTP=1`), a real broker (`XEWE_TEST_HA_MQTT_HOST`: discovery, echoes, mode and
  parameters, cleanup), the first-boot wait after `reset` and `disable`/`enable`
  (`XEWE_TEST_HA_ROUND_TRIP=1`, mDNS with `XEWE_TEST_HA_MDNS=1`), the web interface setup boot
  (`XEWE_TEST_HA_WEB_RESET=1`) and a fresh device (`XEWE_TEST_HA_FACTORY_RESET=1`). The module
  docstring names what each group changes on the board. Without a broker the tests that need one
  skip with `needs an MQTT broker (…)` (`XEWE_TEST_HA_MQTT_HOST` unset, or the module disabled).
