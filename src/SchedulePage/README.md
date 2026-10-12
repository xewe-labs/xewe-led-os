<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# Schedule Page (`schedule_page`)

The schedule editor in a browser. It serves the schedule page, its stylesheet and scripts and
three JSON routes on the web interface module's HTTP server. The schedule blocks themselves
belong to the Scheduler module (`$scheduler ...`); this page only lists, adds, edits and deletes
them.

| | |
|---|---|
| id / name | `schedule_page` / `Schedule Page` |
| declare | `SchedulePage schedule_page(os, web_interface, scheduler);` after `src/Modules.h` |
| requires | Led Web (`led_web`, looked up by id in the constructor: declare after it) |
| first-boot setup | none, no prompt |
| can be disabled | no (it follows Led Web: disabling Led Web takes the page away on the next boot); the same flags as Web Fallback |
| commands | none (state in `$system status`) |
| stored data | none of its own |

## Routes

Registered in `begin_routines_regular` (every boot: the module has no first-boot setup) on
`web_interface.get_server()`, the same server Led Web uses. Every answer carries
`Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE, PUT`
and `Access-Control-Allow-Headers: Content-Type`; no caching headers.

| Path | Method | Answer |
|---|---|---|
| `/schedule`, `/templates/schedule.html` | GET | 200 `text/html`, the page |
| `/schedule/json` | GET | 200 `application/json`, the scheduler's blocks (`[]` when it is disabled) |
| `/schedule/set` | POST | see below |
| `/schedule/delete` | POST | see below |
| `/static/schedule-style.css`, `/%7B%7B%20url_for('static',%20filename='style.css')%20%7D%7D` | GET | 200 `text/css` |
| `/static/schedule-core.js`, `-actions.js`, `-interactions.js`, `-ui.js` | GET | 200 `application/javascript` |
| `/static/schedule-utils.js` | GET | 200 `application/javascript`, empty body |

`POST /schedule/set` with the JSON body `{id?, commands, displayed_color, day, start_time,
end_time}`:

- no body: 400 `{"status": "error", "message": "No payload"}`; unparsable:
  400 `{"status": "error", "message": "Invalid JSON"}`
- empty commands are dropped; values are not range-checked (day, start and end are narrowed to
  8 / 16 / 16 bits)
- an `id` greater than 0 deletes that block first, so the edit replaces it. **`id` 0 deletes
  nothing: editing block 0 adds a second block next to it.**
- the scheduler refuses the add only while it is disabled: 400
  `{"status": "error", "message": "Rejected"}`; else 200 `{"status": "success"}`. The scheduler
  is disabled only from boot on (when a module it requires is disabled), and then it loads no
  blocks: the edit's delete finds nothing and the blocks stored on the device stay as they are.

`POST /schedule/delete` with `{id}`: no body 400 `No payload`; unparsable or no `id`: 400
`{"status": "error", "message": "Invalid ID"}`; else 200 `{"status": "success"}`, also when no
block has that id.

The handlers live in `SchedulePageApi.h` (no server, host-tested); `SchedulePage.cpp` is the glue.

## The page

`pages/*.h` hold the page bytes unchanged. A new block's default command is
`$led set_hsv 85 255 255` with the colour `#33ff33`. The Home link goes to `/`, which is Led Web's
LED page.

## Known issues

- Through `/templates/schedule.html` the page's relative script links point under
  `/templates/static/`, which is not routed, so that address shows the page without its scripts.
- With Led Web disabled every boot prints `Schedule Page module requires:` /
  `Led Web, use: $led_web enable`, and the schedule routes are not registered.
- `$led_web disable` lists Schedule Page among its dependents and prints
  `Schedule Page module reset and disabled`, but the page is neither reset nor disabled: it cannot
  be disabled, and it is simply not served from the next boot on, until Led Web is enabled again.
- It cannot be disabled and has no commands, the same as Web Fallback: there is no
  `$schedule_page` group, and `$system status` shows it enabled whenever Led Web is.
  `$system reset` prints `Schedule Page module reset` (it holds no data of its own).
- No page on the boot right after `$web_interface reset` (or after adding the web interface to a
  device): the web interface starts its server only on boots after its first-boot setup, so the
  routes are registered but nothing answers until one more restart. Led Web's LED page has the
  same gap. Exceptions: with Alexa enabled, Alexa starts the shared server on that boot, and so
  does Home Assistant's first-boot wait; then the page is served.
- With the scheduler disabled the page loads, the list is empty and every save answers
  `Rejected`; deleting still answers success.
- A preflight (`OPTIONS`) to these routes is answered by the server's catch-all (Web Fallback, or
  Alexa when enabled), not by this module.
