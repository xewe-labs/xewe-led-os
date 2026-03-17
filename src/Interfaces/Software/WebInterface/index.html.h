#pragma once

#include <pgmspace.h>

static const char WEB_INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
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

    <div class="control" id="additional-toggle-wrap" style="display:none;">
      <button id="toggleAdditional" type="button" class="secondary">Show Additional Parameters</button>
    </div>

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
  </section>

  <script src="/index.js" defer></script>
</body>
</html>
)rawliteral";