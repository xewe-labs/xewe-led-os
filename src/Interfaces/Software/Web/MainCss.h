// Software/Web/MainCss.h
#pragma once
static const char* STYLES_CSS = R"CSS(:root{
  /* Theme */
  --bg: #0f1216;
  --text: #e8eaed;
  --muted: #c2c7cf;
  --panel: #171b21;
  --control: #1e252e;
  --button-idle: #252e39;
  --button-active: #10161c;
  --divider: #2a3138;
  --focus: #39424a;
  color-scheme: dark;

  /* Power/Status indicators */
  --indicator-green: rgb(43,200,64);
  --indicator-red: rgb(200,64,64);

  /* Typography */
  --font-main: 16px;
  --font-subprime: 14px;
  --line: 1.45;

  /* Layout heights (compacted) */
  --row-h: 44px;
  --ctl-h: 36px;

  /* Slider sizing */
  --track-h: 6px;
  --thumb-d: var(--ctl-h);
}

* { box-sizing: border-box; }
html, body {
  margin: 0;
  padding: 0;
  font-family: system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif;
  font-size: var(--font-subprime);
  line-height: var(--line);
  background: var(--bg);
  color: var(--text);
  -webkit-text-size-adjust: 100%;
  text-size-adjust: 100%;
}

/* Page header */
.page-header {
  border-bottom: 1px solid var(--divider);
  background: var(--bg);
}
.page-title {
  margin: 6px 0;
  font-weight: 800;
  font-size: 22px;
  color: var(--text);
  text-align: center;
}

/* Full-width content */
.wrap {
  width: 100%;
  max-width: none;
  margin: 0;
  padding: 12px 24px;
}

/* Control block card */
.block{
  background: var(--panel);
  border: 1px solid var(--divider);
  border-radius: 16px;
  padding: 0 10px;
}
.block + .block{ margin-top: 8px; }

/* Title with inline status */
.block-title{
  margin: 0;
  padding: 8px 0;
  font-weight: 800;
  font-size: 18px;
  color: var(--text);
  border-bottom: 1px solid var(--divider);

  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}
.status{
  display: inline-flex;
  align-items: center;
  gap: 6px;
}
.status .dot{
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: var(--indicator-red);
  box-shadow: 0 0 0 2px var(--panel), 0 0 0 1px var(--divider);
}
.status[data-online="true"] .dot{
  background: var(--indicator-green);
}

/* Rows now single-column (no labels) */
.row {
  display: grid;
  grid-template-columns: minmax(0, 1fr);  /* allow child to truly fill */
  gap: 12px;
  align-items: center;
  min-height: var(--row-h);
  padding: 8px 10px;
  border-bottom: 1px solid var(--divider);
}
.row:last-child { border-bottom: 0; }
.row > * { width: 100%; min-width: 0; }   /* prevent intrinsic sizing on iOS */

/* Shared control sizing */
input[type="range"],
.seg,
.seg button,
#mode { block-size: var(--ctl-h); }

input[type="range"]{
  display: block;            /* fix iOS intrinsic width */
  width: 100%;
  min-width: 0;
  appearance: none;
  -webkit-appearance: none;
  touch-action: pan-y;       /* avoid horizontal scrolling conflicts */
}

/* Segmented radio-style switch */
.seg {
  display: grid;
  grid-template-columns: 1fr 1fr;
  width: 100%;
  border-radius: 12px;
  background: var(--control);
  overflow: hidden;
}
.seg button {
  position: relative;
  appearance: none;
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  padding: 0 12px 0 36px;
  font: inherit;
  font-size: var(--font-subprime);
  background: var(--button-idle);
  color: var(--muted);
  border: none;
  cursor: pointer;
}
.seg button + button { border-left: 1px solid var(--divider); }

/* indicator dot in segmented control */
.seg button::before{
  content:"";
  position:absolute;
  left:12px; top:50%; transform:translateY(-50%);
  width:14px; height:14px; border-radius:50%;
  box-shadow: inset 0 0 0 2px #6a6a6a;
  background: transparent;
}

/* ACTIVE (locked) */
.seg button[aria-checked="true"]{
  background: var(--button-active);
  color:#ffffff;
  cursor: default;
}
.seg button[aria-checked="true"]::before{
  background: var(--indicator-green);
  box-shadow: inset 0 0 0 2px var(--indicator-green);
}
.seg button[aria-checked="false"]{
  background: var(--button-idle);
  color: var(--muted);
}
.seg button[disabled]{ pointer-events: none; }

/* Color slider */
#color {
  height: var(--ctl-h);
  border-radius: calc(var(--ctl-h) / 2);
  outline: none;
  background: linear-gradient(90deg,
    hsl(0 100% 50%),
    hsl(60 100% 50%),
    hsl(120 100% 40%),
    hsl(180 100% 40%),
    hsl(240 100% 60%),
    hsl(300 100% 50%),
    hsl(360 100% 50%)
  );
}
#color::-webkit-slider-runnable-track { height: var(--track-h); border-radius: 999px; background: inherit; }
#color::-moz-range-track            { height: var(--track-h); border-radius: 999px; background: inherit; }
#color::-webkit-slider-thumb{
  -webkit-appearance: none;
  appearance: none;
  width: var(--thumb-d); height: var(--thumb-d);
  border-radius: 50%;
  background: #fff; border: 2px solid #000;
  /* center knob vertically over thin track */
  margin-top: calc((var(--track-h) - var(--thumb-d)) / 2);
}
#color::-moz-range-thumb{
  width: var(--thumb-d); height: var(--thumb-d);
  border-radius: 50%;
  background: #fff; border: 2px solid #000;
}

/* Brightness */
#brightness {
  --h: 200;
  height: var(--ctl-h);
  border-radius: calc(var(--ctl-h) / 2);
  outline: none;
  background: linear-gradient(90deg,
    #000 0%,
    hsl(var(--h) 100% 10%) 12%,
    hsl(var(--h) 100% 50%) 100%);
}
#brightness::-webkit-slider-runnable-track { height: var(--track-h); border-radius: 999px; background: inherit; }
#brightness::-moz-range-track            { height: var(--track-h); border-radius: 999px; background: inherit; }
#brightness::-webkit-slider-thumb{
  -webkit-appearance: none;
  appearance: none;
  width: var(--thumb-d); height: var(--thumb-d);
  border-radius: 50%;
  background: #fff; border: 2px solid #000;
  margin-top: calc((var(--track-h) - var(--thumb-d)) / 2);
}
#brightness::-moz-range-thumb{
  width: var(--thumb-d); height: var(--thumb-d);
  border-radius: 50%;
  background: #fff; border: 2px solid #000;
}

/* iOS Safari fine-tuning: ensure full-width and pixel-perfect knob centering */
@supports (-webkit-touch-callout: none) {
  .row { grid-template-columns: minmax(0, 1fr); }
  input[type="range"] { width: 100%; display:block; }
  #color::-webkit-slider-runnable-track,
  #brightness::-webkit-slider-runnable-track { height: var(--track-h); }
  #color::-webkit-slider-thumb,
  #brightness::-webkit-slider-thumb {
    /* slight optical correction on iOS to avoid visible drift */
    margin-top: calc((var(--track-h) - var(--thumb-d)) / 2 - 1px);
  }
}

/* Dropdown (Mode) */
#mode{
  appearance: none;
  -webkit-appearance: none;
  -moz-appearance: none;
  width: 100%;
  height: var(--ctl-h);
  padding: 0 36px 0 12px;
  border: 1px solid var(--divider);
  border-radius: 12px;
  background-color: var(--control);
  color: var(--muted);
  line-height: var(--ctl-h);
  outline: none;
  font-size: var(--font-subprime);
  font-family: inherit;
  background-image: url("data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='%23c2c7cf' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='6 9 12 15 18 9'/></svg>");
  background-repeat: no-repeat;
  background-position: right 12px center;
  background-size: 14px 14px;
}
#mode:focus { border-color: var(--focus); color: var(--text); }
#mode option {
  background: var(--control);
  color: var(--text);
  font-size: var(--font-subprime);
  font-family: inherit;
}
#mode::-ms-value{ color: var(--muted); background: transparent; }

/* Full-width row for single controls */
.row--full { grid-template-columns: 1fr; }

/* Button styled to match controls */
.btn{
  display: inline-flex;
  justify-content: center;
  align-items: center;
  height: var(--ctl-h);
  width: 100%;
  padding: 0 16px;
  border-radius: 12px;
  border: 1px solid var(--divider);
  background: var(--control);
  color: var(--muted);
  text-decoration: none;
  font: inherit;
  font-size: var(--font-subprime);
  cursor: pointer;
}
.btn:hover { color: var(--text); }
.btn:focus { outline: none; border-color: var(--focus); color: var(--text); }
.btn:active { background: var(--button-active); color: #fff; }
.btn::after { content: "›"; margin-left: 8px; opacity: .7; }

/* Mobile tweaks for extra comfort */
@media (max-width: 480px){
  :root{
    --row-h: 50px;
    --ctl-h: 40px;
    --thumb-d: var(--ctl-h);
    --track-h: 8px;
  }
  .wrap { padding: 12px 16px; }
}
)CSS";
