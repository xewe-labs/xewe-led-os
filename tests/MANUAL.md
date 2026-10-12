<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Manual test plan: real devices

These checks need a person with a phone, an Echo and an MQTT broker, or they cut the power. The
automated board tests (`tests/board/`, `xewe test`) cover the console, settings, HTTP and restarts;
everything below crosses into another device or cannot be scripted.

Write the result of each check in the table at the end: pass, fail (with the console output), or
skipped (with the reason).

## What you need

- One board with a strip on USB (the Led Dock V2 pins are the defaults), a computer with this
  repository set up (`./setup.sh`).
- A 2.4 GHz Wi-Fi network without client isolation. Phone, Echo, broker and computer on it.
- The web interface on port 80 (`$web_interface get port`): Alexa and the Home Assistant
  discovery only work there.
- An iPhone with the Home app, and a home hub (HomePod or Apple TV) for the remote checks.
- An Echo speaker.
- An MQTT broker. Without Home Assistant, start one on the computer:

  ```sh
  docker run --rm -p 1883:1883 eclipse-mosquitto:2 mosquitto -c /mosquitto-no-auth.conf
  ```

  and watch all device traffic in a second terminal:

  ```sh
  mosquitto_sub -h localhost -v -t 'xewe_led_os/#' -t 'homeassistant/#'
  ```

In the steps, `IP` is the device address (`$led_web status` prints `Page: http://IP/`), `HOST` the
computer running the broker, `DEV` the `xewe_led_os_<12 hex digits>` id in the first
`homeassistant/light/DEV/config` topic, and `BASE` is `xewe_led_os/DEV`. `NAME` is the device name.

## 1. HomeKit

| # | Do | Expect |
|---|---|---|
| 1.1 | On a device where HomeKit is enabled but not paired (first boot, or after `$homekit reset`), read the console. | The pairing wait prints the QR link and `using code 4663-7726`, then `TO ABORT PRESS (x): `. The strip keeps animating. |
| 1.2 | Home app: **+**, Add Accessory, More options, `NAME`, Add Anyway, code `46637726`. | `Setting up HomeKit`, dots for 3 s, `Device successfully paired with HomeKit.`; the boot continues. |
| 1.3 | Toggle the tile, move brightness, pick a colour (mode 0, Solid). | The strip follows each change: colour, brightness and state are applied on every write. |
| 1.4 | `$led set_brightness 128` on the console. | The Home app shows 50 %. |
| 1.5 | `$led set_brightness 0`, look at the tile; then `$led set_brightness 128`. | The accessory stays reachable (no "Not Responding") with brightness below the 1 % minimum; same as 2.3.0. |
| 1.6 | **Reset while disabled (F1 on hardware).** `$homekit disable`, answer `y`; after the reboot `$homekit reset`. | `You also need to remove the device from the Home App manually`, `HomeKit module reset`, `Rebooting`, then the pairing wait of the next boot. No `Guru Meditation`, `panic`, `abort()` or `Backtrace` anywhere in between. Type `x` if you do not continue with 1.7. |
| 1.7 | **Re-pairing after the reset.** In the Home app remove the old `NAME` accessory, then add it again as in 1.2. | Pairing succeeds. The tile shows the brightness it had before 1.6 (the reset erases only the pairing list, not the stored values). |
| 1.8 | Phone on mobile data (home hub present), toggle the tile. | The strip follows (remote access through the hub). |
| 1.9 | **`$system reset` with HomeKit enabled and paired**, answer `y`. Do this last: it erases every setting. | The reset lines, `Rebooting`, then the first-boot questions from the device name on. No `Guru Meditation` between the erase and the restart. The Home app tile goes "No Response"; remove it by hand. |
| 1.10 | Repeat 1.9 with HomeKit **disabled** (`$homekit disable` first). | As 1.9: no crash, the next boot is a first boot. |

## 2. Alexa

| # | Do | Expect |
|---|---|---|
| 2.1 | On the discovery wait (first boot, or after `$alexa reset`): "Alexa, discover devices". | Alexa finds `NAME`; the console prints `Setting up Alexa`, dots for 3 s, `Device successfully paired with Alexa`. |
| 2.2 | "Alexa, turn off NAME", "turn on". | The strip turns off and on. Turning off keeps the brightness. |
| 2.3 | "Alexa, set NAME to 20 percent". "Alexa, set NAME to green" (mode 0). | Dimmer; then solid green. |
| 2.4 | `$led turn_off`, look at the Alexa app. | The app shows the light off. |
| 2.5 | **First "on" after a boot (B-5).** `$led turn_off`, `$system restart`; before changing any colour, "Alexa, turn on NAME". | The strip takes Espalexa's default device colour, not the colour it had before the restart: no LED state reaches Alexa at boot. 2.3.0 behaves the same. Write down the colour. Restore with `$led set_rgb`. |
| 2.6 | **Server on the web interface's setup boot (B-3).** With Alexa enabled: `$web_interface reset`. On the boot after it: `curl -s http://IP/description.xml`, `curl -s -o /dev/null -w '%{http_code}\n' http://IP/`. Then `$system restart` and repeat. | First boot: both answer on port 80 (Espalexa started the shared server); the web interface's own routes do not answer on that boot. After the restart everything answers as usual. |
| 2.7 | `$system restart`, wait 30 s, "Alexa, turn on NAME". | Works without a new discovery. |

## 3. Home Assistant (MQTT)

Provision the broker first: `curl -X POST http://IP/provision -d '{"host":"HOST","port":1883}'`
answers `{"ok":true}` and `$homeassistant status` shows `broker HOST:1883 connected`.

| # | Do | Expect |
|---|---|---|
| 3.1 | **HomeKit to Home Assistant echo.** With HomeKit paired, change colour, brightness and on/off in the Home app. | `BASE/light/state` follows each change on `mosquitto_sub` (the HomeKit write reaches the led, and Home Assistant publishes every LED change, whatever its source). |
| 3.2 | **Last will.** While connected, cut the board's power (pull USB). | `BASE/avail` turns `offline` (retained) after the broker's keep-alive timeout (about 20 s). Plug in again: `avail` `online` and the states are published again. |
| 3.3 | **Blocking reconnect.** Stop the broker for one minute. Meanwhile use the LED page, the Home app and Alexa, and time `curl -s -o /dev/null -w '%{time_total}\n' http://IP/state` a few times. | Every 3 s the module tries to connect and blocks for the TCP/MQTT timeout: the page, HomeKit and Alexa pause meanwhile (2.3.0 behaviour). Write down the longest pause. After the broker is back the device reconnects by itself. |
| 3.4 | In Home Assistant (if available): Settings, Devices & services, MQTT. | Device `NAME` with a light, a Mode select and one number entity per parameter of the running mode. |
| 3.5 | `curl -X POST http://IP/deprovision` | `{"ok":true}`; every retained discovery and state topic of `DEV` is emptied and `BASE/avail` is `offline`; Home Assistant removes the device. |

## 4. Everything together

| # | Do | Expect |
|---|---|---|
| 4.1 | All integrations connected. Change the colour in the Home app (Solid). | LED page, Alexa app and the Home Assistant light show the new colour. |
| 4.2 | Alexa: "turn off". | Home app, LED page and Home Assistant show off. |
| 4.3 | Home Assistant: switch Mode to Pulse. | The LED page shows Pulse with its sliders. |
| 4.4 | Change brightness quickly on the LED page for 30 s. | No restart, no stuck page; `$system status` still answers. |

## Results

| Check | Board / chip | Firmware | Result | Notes |
|---|---|---|---|---|
| 1.1–1.10 | | | | |
| 2.1–2.7 | | | | |
| 3.1–3.5 | | | | |
| 4.1–4.4 | | | | |
