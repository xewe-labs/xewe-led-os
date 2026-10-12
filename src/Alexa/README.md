<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Alexa (project module of xewe-led-os)

Amazon Alexa control of the LED strip through Espalexa (a Philips Hue bridge emulation). It behaves
as the Alexa module of xewe-led-os 2.3.0.

| | |
|---|---|
| id / CLI group / NVS namespace | `alexa` |
| name | `Alexa` |
| first-boot question | yes (`Would you like to enable Alexa module?`), then the discovery wait |
| requires | `wifi` (passed to the constructor; `$wifi disable` disables Alexa too) |
| commands | the generic core commands: `status`, `reset`, `enable`, `disable` (no `set`/`get`/`schema`: the core adds those only for a module with a settings table, and Alexa has none) |
| own NVS keys | none |
| library | Espalexa, xewe-labs fork, pinned in `xewe.toml` |

Declaration in the sketch, after the generated modules and after WebFallback, HomeAssistant and
HomeKit:

```cpp
#include "src/Alexa/Alexa.h"
Alexa alexa(os, wifi, web_interface, led);
```

## What it does

- **Not-found handler.** In `begin_routines_required` (that is, only when Alexa is enabled and wifi
  is enabled) it installs this handler on the web interface's server:

  ```cpp
  server_ref.onNotFound([this, &server_ref]() {
      if (!espalexa.handleAlexaApiCall(server_ref.uri(), server_ref.arg(0))) {
          server_ref.send(404, "text/plain", "Endpoint not found.");
      }
  });
  ```

  Any path containing `api` is answered by Espalexa (200 JSON); everything else gets
  `404 text/plain Endpoint not found.`. No CORS headers and no OPTIONS case, as 2.3.0. A
  `WebServer` keeps only the last not-found handler. The Web Fallback module is declared before
  `alexa` and installs the 2.3.0 web-interface fallback (CORS headers, OPTIONS 204, else 404) on
  every boot; when Alexa is enabled, its `begin_routines_required` runs later and replaces that
  handler with the one above. When Alexa is disabled (stored disable, a first-boot "no", or wifi
  disabled) its routines do not run and the fallback stays. So the handler in force is decided by
  Alexa's state at boot, as in 2.3.0, and changes only across a restart (enable, disable and reset
  always restart).
- **Espalexa on the shared server.** `espalexa.begin(&server)` adds `GET /espalexa` and
  `GET /description.xml` and calls `server.begin()` itself, so the server also runs on the
  boot where the web interface does not start it (the first boot). On that boot the server
  listens on port 80 whatever the web interface's `port` setting, and the web interface's own
  routes are absent. Espalexa's SSDP replies and
  `description.xml` always announce port 80: **Alexa needs the web interface on port 80.**
  `espalexa.loop()` also calls `handleClient()` on the shared server (as 2.3.0).
- **Device.** One `EspalexaDeviceType::color` device named after `os.system.get_device_name()`.
  Renaming the device needs a new discovery in the Alexa app.
- **First-boot discovery wait** (first boot, and the boot after an enable or a reset): prints

  ```
  \nAsk Alexa to discover new devices\nThe setup process will continue automatically\nafter device is pared with Alexa
  TO ABORT PRESS (x): 
  ```

  and loops until an Echo fetched `/description.xml`. A line starting with `x` disables the module
  (no confirmation) and restarts; any other line prints `\n(x)?: `. Then `Setting up Alexa`, 3 s of
  dots, `\nDevice successfully paired with Alexa`. It blocks without a timeout, so `xewe provision`
  cannot drive it.
- **Alexa -> LED** (Espalexa callback): `led.sync_state(on)`, then `led.sync_brightness(value)`
  only while on (so turning off keeps the brightness), then `led.sync_color(r, g, b)`, all with
  origin `this`.
- **LED -> Alexa** (`LedListener`, registered in `begin_routines_required`, so a disabled Alexa
  receives nothing; events with origin `this` are ignored):
  - colour: `hsv = rgb_to_hsv(rgb)`, `setColor(h << 8 | h, s == 255 ? 254 : s)` (`AlexaMap.h`);
  - brightness: `setValue(b)`, also while the LED is off; a non-zero value then makes Alexa report
    the light as on (as 2.3.0);
  - state: `setState(on)`;
  - mode, length: nothing;
  - all (`$led reset`): state, brightness, colour.
- **status**: the core line `Alexa module enabled|disabled`, nothing else (2.3.0 had no override).
- **reset** (also inside disable and `$system reset`): prints
  `You also need to remove the device from the Alexa App manually.` (always, also when not
  verbose), then the core reset.

## Known issues

- Alexa needs the web interface on port 80 (Espalexa hard-codes it in SSDP and description.xml).
- The LED keeps animating during the discovery wait (the led module renders in its own task;
  2.3.0 froze the strip).
- On a fresh device the first-boot questions come in the generated module order (led, wifi,
  time, scheduler, buttons before the integrations), not the 2.3.0 order.
- No LED state is sent to Alexa at boot (as 2.3.0): the first Alexa "on" before any LED colour
  change applies Espalexa's default device colour to the LED.

## Tests

- `tests/unit/test_alexa.py` with `tests/unit/host_alexa_map.cpp`: colour mapping (g++), 2.3.0
  literals for colour -> hue16/sat8, the whole body of every `Alexa::` function compared statement
  by statement (handler, begin order, discovery wait, callbacks, loop, reset), the constructor's
  wifi requirement, and the sketch declaration (uncommented, after `web_fallback`).
- `tests/board/test_alexa.py`: status; with `XEWE_TEST_ALEXA_HTTP=1` the not-found handler, the
  Espalexa API both ways (including hue/sat), an `x` abort of the discovery wait and a
  disable/enable cycle (each checks the Web Fallback answer while Alexa is disabled); with
  `XEWE_TEST_ALEXA_ECHO=1` discovery by a real Echo.
