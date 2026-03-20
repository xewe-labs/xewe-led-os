#pragma once
#include <pgmspace.h>
static const char SCHEDULE_UTILS_JS[] PROGMEM = R"rawliteral(
// calendar-utils.js
(function () {
    // Global App Initialization
    window.CalendarApp = window.CalendarApp || {
        calendar: document.getElementById('calendar'), // Adjust if your selector is different
        config: { startHour: 8, endHour: 24, pixelsPerHour: 100, days: ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'] },
        state: { eventDatabase: {}, currentDragSession: new Set(), activeEventId: null, menuTargetSlot: null, copiedBlockData: null },
        els: {}, utils: {}, ui: {}, actions: {}, interactions: {}
    };

    const app = window.CalendarApp;

    Object.assign(app.utils, {
        generateEventId: () => 'evt_' + Date.now().toString(36) + Math.random().toString(36).substring(2, 7),
        timeToRow: (time) => {
            const [h, m] = time.split(':').map(Number);
            return (h - app.config.startHour) * 4 + Math.floor(m / 15);
        },
        rowToTime: (row) => {
            const h = Math.floor(row / 4) + app.config.startHour;
            return `${h}:${((row % 4) * 15).toString().padStart(2, '0')}`;
        },
        getTotalRows: () => (app.config.endHour - app.config.startHour) * 4,
        getSlotByDayRow: (day, row) => app.calendar.querySelector(`.slot[data-day="${day}"][data-row="${row}"]`),
        getTodayIndex: () => new Date().getDay(),
        getCalendarHeaderHeightPx: () => 60,

        // Auto-color detection logic (customize your keywords here)
        detectColorFromCommands: (cmdStr) => {
            const str = cmdStr.toLowerCase();
            if (str.includes('$led set_hsv')) return '#33ff33';
            if (str.includes('error') || str.includes('stop')) return '#ff3b30';
            return null; // Return null to fallback to default
        }
    });
})();
)rawliteral";
