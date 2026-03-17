#pragma once

#include <pgmspace.h>

static const char WEB_INDEX_CSS[] PROGMEM = R"rawliteral(
:root {
  --bg:#1a1a1a;
  --fg:#f0f0f0;
  --accent:#0af;
  --green:#0f0;
  --red:#f00;
  --font:system-ui, sans-serif;
  --radius:12px;
  --thumb-size:26px;
  --track-height:12px;
  --outline:#3a3a3a;
}

*, *::before, *::after {
  box-sizing: border-box;
  margin:0;
  padding:0;
}

body {
  background: var(--bg);
  color: var(--fg);
  font-family: var(--font);
  display:flex;
  flex-direction:column;
  align-items:center;
  padding:1rem;
  min-height:100vh;
  gap:1.25rem;
}

.panel {
  width:80vw;
  display:flex;
  flex-direction:column;
  align-items:stretch;
  gap:1rem;
}

h1 {
  font-weight:500;
}

#status {
  display:flex;
  align-items:center;
  gap:.5rem;
}

#status-indicator {
  width:12px;
  height:12px;
  border-radius:50%;
  background:var(--red);
  transition:background .5s ease;
}

.controls-grid {
  display:grid;
  grid-template-columns:1fr;
  gap:1.25rem;
  width:100%;
  max-width:none;
}

.control {
  display:flex;
  flex-direction:column;
  gap:0.25rem;
}

select {
  width:100%;
  appearance:none;
  background:transparent;
  border:1px solid var(--fg);
  border-radius:5px;
  color:var(--fg);
  padding:.5rem;
}

.buttons {
  display:grid;
  grid-template-columns:repeat(auto-fit, minmax(100px, 1fr));
  gap:.5rem;
  width:100%;
  max-width:none;
}

button {
  padding:.75rem;
  background:var(--accent);
  border:none;
  border-radius:5px;
  color:var(--bg);
  font-size:1rem;
  font-weight:500;
  cursor:pointer;
  transition:opacity .2s ease;
}

button:disabled {
  opacity:.4;
  cursor:not-allowed;
}

button.secondary {
  background:transparent;
  color:var(--fg);
  border:1px solid var(--outline);
}

.range-wrap {
  position:relative;
  display:grid;
  align-items:center;
}

.bubble {
  position:absolute;
  right:0;
  top:-22px;
  font-size:.8rem;
  color:#b7bdc9;
  pointer-events:none;
}

.param-label {
  text-align:center;
  font-size:0.85rem;
  color:#cfd2d8;
  font-weight:500;
}

input[type=range].range {
  -webkit-appearance:none;
  appearance:none;
  width:100%;
  height:var(--thumb-size);
  background:transparent;
  margin:6px 0;
  touch-action:none;
  border:none;
}

input[type=range].range::-webkit-slider-runnable-track {
  height:var(--track-height);
  background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));
  border-radius:999px;
  border:1px solid var(--outline);
}

input[type=range].range::-webkit-slider-thumb {
  -webkit-appearance:none;
  appearance:none;
  width:var(--thumb-size);
  height:var(--thumb-size);
  border-radius:50%;
  border:2px solid rgba(0,0,0,.25);
  background:var(--thumb-bg,#fff);
  box-shadow:0 4px 10px rgba(0,0,0,.45);
  margin-top:calc((var(--track-height) - var(--thumb-size))/2);
}

input[type=range].range::-moz-range-track {
  height:var(--track-height);
  background:var(--track-bg,linear-gradient(90deg,#3b3f52,#3b3f52));
  border-radius:999px;
  border:1px solid var(--outline);
}

input[type=range].range::-moz-range-thumb {
  width:var(--thumb-size);
  height:var(--thumb-size);
  border-radius:50%;
  border:2px solid rgba(0,0,0,.25);
  background:var(--thumb-bg,#fff);
  box-shadow:0 4px 10px rgba(0,0,0,.45);
}

input[type=range].hue {
  --track-bg:linear-gradient(
    to right,
    hsl(0,100%,50%) 0%,
    hsl(60,100%,50%) 16.6%,
    hsl(120,100%,45%) 33.3%,
    hsl(180,100%,45%) 50%,
    hsl(240,100%,50%) 66.6%,
    hsl(300,100%,50%) 83.3%,
    hsl(360,100%,50%) 100%
  );
}

input[type=range].generic {
  --track-bg:linear-gradient(90deg, #3b3f52, #5a6288);
  --thumb-bg:#fff;
}

hr {
  border:0;
  height:1px;
  background:#333;
  margin:0.5rem 0;
}
)rawliteral";