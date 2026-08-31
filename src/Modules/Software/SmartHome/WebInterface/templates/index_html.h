// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/SmartHome/WebInterface/templates/index_html.h
#pragma once

#include <pgmspace.h>


static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>XeWe LED</title>
  <link rel="stylesheet" href="/index.css">
</head>
<body>
  <section class="panel">
    <h1 id="device-title">Loading…</h1>
    <div id="status">
      <div id="status-indicator"></div>
      <span id="status-text">Offline</span>
    </div>

    <div class="control" style="margin-bottom:0.25rem;">
      <select id="mode" aria-label="Mode">
        <option value="0">Loading...</option>
      </select>
    </div>

    <div class="controls-grid" id="slider-container"></div>

    <hr>

    <div class="control" id="additional-toggle-wrap" style="display:none;">
      <button id="toggleAdditional" type="button" class="secondary">Show Additional Parameters</button>
    </div>

    <div class="control" id="reset-defaults-wrap" style="display:none;">
      <button id="resetDefaults" type="button" class="secondary">Reset Parameters to Defaults</button>
    </div>

    <hr>

    <div class="control">
      <div class="range-wrap">
        <input
          type="range"
          id="brightness"
          class="range brightness"
          min="0"
          max="255"
          step="1"
          aria-label="Brightness"
        />
        <output id="brightnessValue" class="bubble">0</output>
      </div>
      <span class="param-label">Brightness</span>
    </div>

    <div class="buttons" style="margin-top:0.25rem;">
      <button id="btnOn">On</button>
      <button id="btnOff">Off</button>
    </div>

    <div class="control" style="margin-top: 1rem;">
      <button id="btnSchedule" type="button" class="secondary" onclick="window.location.href='/schedule'">Set Up Schedule</button>
    </div>

  </section>

  <script src="static/index.js" defer></script>
</body>
</html>
)rawliteral";