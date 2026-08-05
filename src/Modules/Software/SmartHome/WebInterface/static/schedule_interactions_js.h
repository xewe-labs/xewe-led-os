#pragma once
#include <pgmspace.h>
static const char SCHEDULE_INTERACTIONS_JS[] PROGMEM = R"rawliteral(
// calendar-interactions.js
(function () {
    const app = window.CalendarApp;
    const { calendar, state } = app;

    function setDraggingUI(enabled) {
        document.body.classList.toggle('calendar-dragging', enabled);
        if (enabled) window.getSelection()?.removeAllRanges();
    }

    function updateDragSelection(slot) {
        if (!state.dragAnchor || !slot || slot.dataset.day !== state.dragAnchor.dataset.day) return;

        const day = state.dragAnchor.dataset.day;
        const sRow = Math.min(state.dragAnchor.dataset.row, slot.dataset.row);
        const eRow = Math.max(state.dragAnchor.dataset.row, slot.dataset.row);
        const nextSession = new Set();

        calendar.querySelectorAll(`.slot[data-day="${day}"][data-row]`).forEach(s => {
            const r = Number(s.dataset.row);
            if (r >= sRow && r <= eRow && !s.dataset.eventId) {
                nextSession.add(s); s.classList.add('selected');
            } else if (state.currentDragSession.has(s)) {
                s.classList.remove('selected');
            }
        });
        state.currentDragSession = nextSession;
    }

    function finishDrag() {
        if (!state.isDragging) return;
        setDraggingUI(state.isDragging = false);
        state.dragAnchor = null;

        if (state.currentDragSession.size > 0) {
            const saved = Array.from(state.currentDragSession);
            app.ui.openModal('Enter CLI Commands', '$led set_hsv 85 255 255', '#33ff33', (cmdText, color) => {
                if (cmdText?.trim()) {
                    app.actions.createEventFromSlots(saved, cmdText.split('\n').map(c => c.trim()).filter(Boolean), color);
                } else {
                    saved.forEach(s => { s.classList.remove('selected'); s.style.backgroundColor = ''; });
                }
            }, false);
        }
        state.currentDragSession.clear();
    }

    Object.assign(app.interactions, {
        bind: () => {
            if (app.interactions._bound) return; app.interactions._bound = true;

            ['resize', 'scroll'].forEach(evt => window.addEventListener(evt, () => app.ui.updateNowIndicator()));
            calendar.addEventListener('scroll', app.ui.updateNowIndicator);

            calendar.addEventListener('pointerdown', (e) => {
                if (app.els.modalOverlay.style.display === 'flex' || e.button !== 0) return;
                const slot = e.target.closest('.slot[data-day][data-row]');
                if (!slot) return;

                if (slot.classList.contains('selected') && slot.dataset.eventId) {
                    setDraggingUI(state.isDragging = false);
                    return app.actions.showMenu(e.clientX, e.clientY, { eventId: slot.dataset.eventId, targetSlot: slot });
                }

                app.actions.hideMenu();
                state.isDragging = true;
                state.dragAnchor = slot;
                state.currentDragSession = new Set();
                setDraggingUI(true);
                e.preventDefault();
                if (calendar.setPointerCapture) calendar.setPointerCapture(e.pointerId);
                updateDragSelection(slot);
            });

            calendar.addEventListener('pointermove', (e) => {
                if (!state.isDragging) return;
                e.preventDefault(); window.getSelection()?.removeAllRanges();
                updateDragSelection(document.elementFromPoint(e.clientX, e.clientY)?.closest('.slot[data-day][data-row]'));
            });

            window.addEventListener('pointerup', finishDrag);
            window.addEventListener('pointercancel', finishDrag);
            calendar.addEventListener('lostpointercapture', finishDrag);

            calendar.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                if (app.els.modalOverlay.style.display === 'flex') return;
                const slot = e.target.closest('.slot');
                if (!slot) return app.actions.hideMenu();
                app.actions.showMenu(e.clientX, e.clientY, { eventId: slot.dataset.eventId || null, targetSlot: slot });
            });
        }
    });

    const loadedDate = new Date().toDateString();
    setInterval(() => { if (new Date().toDateString() !== loadedDate) window.location.reload(); }, 60000);

    // Boot Up Sequence
    app.ui.init();
    app.actions.bindMenuActions();
    app.interactions.bind();
    app.actions.fetchAndLoadSchedules();
})();
)rawliteral";
