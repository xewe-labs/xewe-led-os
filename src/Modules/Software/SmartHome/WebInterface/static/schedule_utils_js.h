#pragma once
#include <pgmspace.h>
static const char SCHEDULE_UTILS_JS[] PROGMEM = R"rawliteral(
(function () {
    // Global App Initialization
    window.CalendarApp = window.CalendarApp || {
        calendar: document.getElementById('calendar'),
        config: { startHour: 8, endHour: 24, pixelsPerHour: 100, days: ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'] },
        state: { eventDatabase: {}, currentDragSession: new Set(), activeEventId: null, menuTargetSlot: null, copiedBlockData: null },
        els: {}, utils: {}, ui: {}, actions: {}, interactions: {}
    };

    const app = window.CalendarApp;

    Object.assign(app.utils, {
        // STRICT NUMERICS ONLY - NO MORE "evt_" PREFIX
        generateEventId: () => Date.now(),

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

        // Auto-color detection logic
        detectColorFromCommands: (cmdStr) => {
            const str = cmdStr.toLowerCase();
            if (str.includes('$led set_hsv')) return '#33ff33';
            if (str.includes('error') || str.includes('stop')) return '#ff3b30';
            return null;
        }
    });
})();
)rawliteral";
