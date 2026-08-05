#pragma once
#include <pgmspace.h>
static const char SCHEDULE_CORE_JS[] PROGMEM = R"rawliteral(
// calendar-core.js

(function () {
    const app = window.CalendarApp = window.CalendarApp || {};

    app.calendar = document.getElementById('calendar');

    app.config = {
        days: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
        startHour: 0,
        endHour: 24,
        pixelsPerHour: 35
    };

    app.state = {
        eventDatabase: {},
        copiedBlockData: null,
        nowLineTimer: null,
        modalCallback: null,
        activeEventId: null,
        menuTargetSlot: null,
        isDragging: false,
        dragAnchor: null,
        currentDragSession: new Set()
    };

    app.els = {};
    app.ui = app.ui || {};
    app.actions = app.actions || {};
    app.interactions = app.interactions || {};

    function clamp(val, min, max) {
        return Math.min(Math.max(val, min), max);
    }

    function rgbToHex(r, g, b) {
        r = clamp(parseInt(r, 10), 0, 255);
        g = clamp(parseInt(g, 10), 0, 255);
        b = clamp(parseInt(b, 10), 0, 255);
        return "#" + (1 << 24 | r << 16 | g << 8 | b).toString(16).slice(1).toUpperCase();
    }

    function hsvToHex(h, s, v) {
        h = clamp(parseInt(h, 10), 0, 255);
        s = clamp(parseInt(s, 10), 0, 255);
        v = clamp(parseInt(v, 10), 0, 255);

        let h_deg = (h / 255) * 360;
        let s_norm = s / 255;
        let v_norm = v / 255;

        let c = v_norm * s_norm;
        let x = c * (1 - Math.abs((h_deg / 60) % 2 - 1));
        let m = v_norm - c;
        let r = 0, g = 0, b = 0;

        if (h_deg >= 0 && h_deg < 60) { r = c; g = x; b = 0; }
        else if (h_deg >= 60 && h_deg < 120) { r = x; g = c; b = 0; }
        else if (h_deg >= 120 && h_deg < 180) { r = 0; g = c; b = x; }
        else if (h_deg >= 180 && h_deg < 240) { r = 0; g = x; b = c; }
        else if (h_deg >= 240 && h_deg < 300) { r = x; g = 0; b = c; }
        else if (h_deg >= 300 && h_deg <= 360) { r = c; g = 0; b = x; }

        r = Math.round((r + m) * 255);
        g = Math.round((g + m) * 255);
        b = Math.round((b + m) * 255);

        return rgbToHex(r, g, b);
    }

    function detectColorFromCommands(text) {
        const lines = text.split('\n');

        for (let i = lines.length - 1; i >= 0; i--) {
            const line = lines[i].trim();

            const rgbMatch = line.match(/\$led\s+set_rgb\s+(\d+)\s+(\d+)\s+(\d+)/i);
            if (rgbMatch) return rgbToHex(rgbMatch[1], rgbMatch[2], rgbMatch[3]);

            const hsvMatch = line.match(/\$led\s+set_hsv\s+(\d+)\s+(\d+)\s+(\d+)/i);
            if (hsvMatch) return hsvToHex(hsvMatch[1], hsvMatch[2], hsvMatch[3]);
        }

        return null;
    }

    function getTodayIndex() {
        const jsDay = new Date().getDay();
        return jsDay === 0 ? 6 : jsDay - 1;
    }

    function getTotalRows() {
        return (app.config.endHour - app.config.startHour) * 4;
    }

    function timeToRow(time) {
        const [hour, minute] = time.split(':').map(Number);
        return ((hour - app.config.startHour) * 4) + Math.floor(minute / 15);
    }

    function rowToTime(row) {
        const hour = Math.floor(row / 4) + app.config.startHour;
        const quarter = row % 4;
        const minute = quarter === 0 ? '00' : quarter === 1 ? '15' : quarter === 2 ? '30' : '45';
        return `${hour}:${minute}`;
    }

    function getSlotByDayRow(day, row) {
        return app.calendar.querySelector(`.slot[data-day="${day}"][data-row="${row}"]`);
    }

    function generateEventId() {
        return 'evt-' + Date.now() + '-' + Math.floor(Math.random() * 1000000);
    }

    function getCalendarHeaderHeightPx() {
        const rootStyles = getComputedStyle(document.documentElement);
        const cssVar = rootStyles.getPropertyValue('--calendar-header-height').trim();
        const cssVarPx = parseFloat(cssVar);

        if (!Number.isNaN(cssVarPx) && cssVarPx > 0) {
            return cssVarPx;
        }

        const header = app.calendar ? app.calendar.querySelector('.day-header') : null;
        if (header) {
            const measured = header.getBoundingClientRect().height;
            if (measured > 0) return measured;
        }

        return 60;
    }

    app.utils = {
        clamp,
        rgbToHex,
        hsvToHex,
        detectColorFromCommands,
        getTodayIndex,
        getTotalRows,
        timeToRow,
        rowToTime,
        getSlotByDayRow,
        generateEventId,
        getCalendarHeaderHeightPx
    };
})();
)rawliteral";
