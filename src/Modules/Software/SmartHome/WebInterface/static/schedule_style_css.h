#pragma once
#include <pgmspace.h>
static const char SCHEDULE_STYLE_CSS[] PROGMEM = R"rawliteral(
:root {
    --bg-color: #121212;
    --surface-color: #1c1c1e;
    --border-color: #38383a;
    --border-light: #2c2c2e;
    --text-main: #f5f5f5;
    --text-muted: #98989d;
    --event-bg: rgba(255, 59, 48, 0.25);
    --event-border: #ff453a;
    --header-h: 40px;
    --rail-w: 31px;
}

*, *::before, *::after { box-sizing: border-box; }

body {
    margin: 0;
    background: var(--bg-color);
    color: var(--text-main);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    overscroll-behavior-y: none;
}

#app {
    display: flex;
    flex-direction: column;
    height: 100vh;
    padding: 20px;
    max-width: 1200px;
    margin: 0 auto;
}

h2 { margin: 0 0 20px; font-weight: 600; }

/* --- Layout --- */
.calendar-shell {
    flex: 1;
    display: flex;
    align-items: stretch;
    gap: 10px;
    min-height: 0;
}

.calendar-time-rail {
    width: var(--rail-w);
    min-width: var(--rail-w);
    overflow: hidden;
    pointer-events: none;
    user-select: none;
}

.calendar-time-rail-inner { will-change: transform; }
.calendar-time-rail-row { color: var(--text-muted); font-size: 11px; position: relative; }
.calendar-time-rail-row span { position: absolute; top: -5px; right: 0; background: var(--bg-color); white-space: nowrap; }

.calendar-container {
    flex: 1;
    display: grid;
    /* Grid templates are dynamically injected by calendar-ui.js */
    overflow: auto;
    border: 1px solid var(--border-color);
    background: var(--bg-color);
    touch-action: none;
    user-select: none;
    position: relative;
}

/* --- Headers & Grid --- */
.day-header {
    border-right: 1px solid var(--border-color);
    border-bottom: 1px solid var(--border-color);
    background: var(--surface-color);
    position: sticky;
    top: 0;
    z-index: 10;
    display: flex;
    align-items: center;
    justify-content: center;
    height: var(--header-h);
}

.day-name { font-size: 11px; text-transform: uppercase; color: var(--text-muted); font-weight: 500; letter-spacing: .04em; }
.day-header.today .day-name { color: var(--event-border); }

.slot { border-right: 1px solid var(--border-color); border-bottom: 1px solid var(--border-light); cursor: crosshair; position: relative; }
.slot:nth-child(even) { border-bottom-color: var(--border-color); }
.slot.hour-marker { border-top: 2px solid #555 !important; }
.calendar-time-rail-row.hour-marker { border-top: none !important; }

/* --- Events --- */
.slot.selected { background: var(--event-bg); border-left: 3px solid var(--event-border); }
.slot[data-event-id] { border-bottom-color: rgba(255, 255, 255, 0.1); }

.slot-text {
    position: absolute;
    inset: 0;
    overflow: hidden;
    font: 11px monospace;
    padding: 4px 6px;
    line-height: 1.5;
    color: #fff;
    text-shadow: 0 1px 3px rgba(0,0,0,.6);
    pointer-events: none;
    z-index: 10;
}

.now-line { position: absolute; border-top: 2px solid #ff3b30; z-index: 30; pointer-events: none; }

/* --- UI Elements: Modals & Menus --- */
.modal-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,.6);
    display: none;
    align-items: center;
    justify-content: center;
    z-index: 2000;
}

.modal-box { background: var(--surface-color); padding: 20px; border-radius: 12px; width: 400px; max-width: 90%; box-shadow: 0 10px 25px rgba(0,0,0,.5); display: flex; flex-direction: column; gap: 15px; }
.modal-title { margin: 0; font-size: 18px; }

.modal-textarea {
    width: 100%;
    padding: 10px;
    border-radius: 8px;
    border: 1px solid var(--border-color);
    background: var(--bg-color);
    color: var(--text-main);
    font: 14px monospace;
    resize: vertical;
}
.modal-textarea:focus { outline: none; border-color: #0a84ff; }

.color-container { display: flex; align-items: center; gap: 10px; font-weight: 500; font-size: 14px; }
.modal-color-input { border: 1px solid var(--border-color); width: 36px; height: 36px; padding: 0; border-radius: 6px; cursor: pointer; background: none; }

.modal-btn-container { display: flex; justify-content: flex-end; gap: 10px; }
.btn-cancel, .btn-save, .menu-btn { border: none; border-radius: 6px; cursor: pointer; }
.btn-cancel { padding: 8px 16px; background: #3a3a3c; color: var(--text-main); }
.btn-cancel:hover { background: #444446; }
.btn-save { padding: 8px 16px; background: #0a84ff; color: #fff; }
.btn-save:hover { background: #007aff; }

.context-menu {
    position: absolute;
    display: none;
    background: var(--surface-color);
    border: 1px solid var(--border-color);
    box-shadow: 0 4px 14px rgba(0,0,0,.5);
    border-radius: 10px;
    padding: 6px;
    flex-direction: column;
    gap: 2px;
    min-width: 140px;
    z-index: 1000;
}
.menu-btn { padding: 8px 12px; background: none; color: var(--text-main); text-align: left; font-size: 14px; }
.menu-btn:hover { background: var(--border-light); }
.menu-btn-delete { color: #ff453a; }

.slot.end-marker {
    height: 0 !important;
    min-height: 0 !important;
    max-height: 0 !important;
    border-bottom: none !important;
    border-right: none !important;
}
)rawliteral";
