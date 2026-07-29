// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/WebInterface/WebInterface.cpp

#include "WebInterface.h"
#include "../../Module/ModuleController.h"


WebInterface::WebInterface(ModuleController& controller)
      : Module(controller,
               /* id                  */ "web_interface",
               /* name                */ "Web_Interface",
               /* description         */ "Allows to interact with other devices on the local network",
               /* requires_init_setup */ true,
               /* can_be_disabled     */ false,
               /* has_cli_cmds        */ true)
{}

void WebInterface::begin_routines_regular (const ModuleConfig& cfg) {
    http_server.on("/", HTTP_GET, std::bind(&WebInterface::serve_main_page, this));
    http_server.on("/cmd", HTTP_GET, std::bind(&WebInterface::handle_command_request, this));
    http_server.begin();
    controller.serial_port.print("Web Interface now available at:\nhttp://" + controller.wifi.get_local_ip());
}
void WebInterface::loop () {
    http_server.handleClient();
}

std::string WebInterface::status (const bool verbose) const {
    if (is_disabled()) return Module::status(verbose);

    std::ostringstream out;

    unsigned long uptime_s = millis() / 1000UL;
    int days  = static_cast<int>(uptime_s / 86400UL);
    int hours = static_cast<int>((uptime_s % 86400UL) / 3600UL);
    int mins  = static_cast<int>((uptime_s % 3600UL) / 60UL);
    int secs  = static_cast<int>(uptime_s % 60UL);

    uint32_t free_heap  = ESP.getFreeHeap();
    uint32_t total_heap = ESP.getHeapSize();
    uint32_t used_heap  = total_heap - free_heap;
    float heap_usage    = (total_heap ? (used_heap * 100.0f) / total_heap : 0.0f);

    out << "--- Web Server Status ---\n";
    out << "  - Uptime:       "
        << days << "d "
        << std::setw(2) << std::setfill('0') << hours << ':'
        << std::setw(2) << std::setfill('0') << mins  << ':'
        << std::setw(2) << std::setfill('0') << secs  << '\n';
    out << "  - Memory Usage: "
        << std::fixed << std::setprecision(2) << heap_usage << "% ("
        << used_heap << " / " << total_heap << " bytes)\n";
    out << "-------------------------\n";

    if (verbose) {
        controller.serial_port.print(out.str());
    }

    return out.str();
}

void WebInterface::serve_main_page() {
    if (is_disabled()) return;
    http_server.send_P(200, "text/html", INDEX_HTML);
}

void WebInterface::handle_command_request() {
    if (is_disabled()) return;

    if (http_server.hasArg("c")) {
        std::string command_text = http_server.arg("c").c_str();

        controller.serial_port.print("Got cmd from web: \n" + command_text);
        controller.command_executor.parse(command_text);

        http_server.send(200, "text/plain", "OK");
    } else {
        http_server.send(400, "text/plain", "Empty Command");
    }
}

// --------------------------------------------------------------------------
// HTML Assets
// --------------------------------------------------------------------------

const char WebInterface::INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>XeWe OS Web Interface</title>
    <style>
        :root {
            --bg: #121212;
            --fg: #e0e0e0;
            --input-bg: #1e1e1e;
            --accent: #00bcd4;
            --border: #333;
        }
        body {
            background-color: var(--bg);
            color: var(--fg);
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            padding: 20px;
            box-sizing: border-box;
        }
        .container {
            width: 100%;
            max-width: 600px;
            text-align: center;
        }
        h1 {
            font-weight: 300;
            letter-spacing: 1px;
            margin-bottom: 2rem;
            color: var(--accent);
        }
        .input-group {
            display: flex;
            gap: 10px;
        }
        input[type="text"] {
            flex-grow: 1;
            padding: 15px;
            border-radius: 5px;
            border: 1px solid var(--border);
            background-color: var(--input-bg);
            color: var(--fg);
            font-size: 16px;
            outline: none;
            transition: border-color 0.2s;
        }
        input[type="text"]:focus {
            border-color: var(--accent);
        }
        button {
            padding: 15px 25px;
            border: none;
            border-radius: 5px;
            background-color: var(--accent);
            color: var(--bg);
            font-weight: bold;
            font-size: 16px;
            cursor: pointer;
            transition: opacity 0.2s;
        }
        button:active { opacity: 0.8; }
        #flash {
            margin-top: 10px;
            height: 20px;
            font-size: 0.8rem;
            opacity: 0;
            transition: opacity 0.5s;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>XeWe OS Web Interface</h1>
        <div class="input-group">
            <input type="text" id="cmdInput" placeholder="Enter command..." autofocus autocomplete="off">
            <button onclick="sendCmd()">Send Command</button>
        </div>
        <div id="flash">Command Sent</div>
    </div>
    <script>
        const input = document.getElementById('cmdInput');
        const flash = document.getElementById('flash');

        input.addEventListener("keypress", function(event) {
            if (event.key === "Enter") {
                event.preventDefault();
                sendCmd();
            }
        });

        function sendCmd() {
            const val = input.value.trim();
            if(!val) return;

            fetch('/cmd?c=' + encodeURIComponent(val))
                .then(r => {
                    if(r.ok) {
                        input.value = '';
                        showFlash('Command Sent');
                    } else {
                        showFlash('Error Sending');
                    }
                })
                .catch(e => showFlash('Connection Error'));
        }

        let flashTimer;
        function showFlash(msg) {
            flash.textContent = msg;
            flash.style.opacity = 1;
            clearTimeout(flashTimer);
            flashTimer = setTimeout(() => {
                flash.style.opacity = 0;
            }, 2000);
        }
    </script>
</body>
</html>
)rawliteral";