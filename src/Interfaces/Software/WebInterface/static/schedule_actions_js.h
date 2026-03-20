#pragma once
#include <pgmspace.h>
static const char SCHEDULE_ACTIONS_JS[] PROGMEM = R"rawliteral(
(function () {
    const app = window.CalendarApp;
    const { calendar, state, els, utils } = app;

    function hideMenu() {
        if (!els.menu) return;
        els.menu.style.display = 'none';
        state.activeEventId = state.menuTargetSlot = null;
    }

    function showMenu(x, y, { eventId = null, targetSlot = null } = {}) {
        state.activeEventId = eventId;
        state.menuTargetSlot = targetSlot;

        const canPaste = !!targetSlot && !targetSlot.dataset.eventId && !!state.copiedBlockData && app.actions.canPasteBlockAt(targetSlot);

        els.editCmdBtn.style.display = els.copyBtn.style.display = els.deleteBtn.style.display = eventId ? 'block' : 'none';
        els.pasteBtn.style.cssText = `display: ${targetSlot ? 'block' : 'none'}; opacity: ${canPaste ? '1' : '0.5'}; cursor: ${canPaste ? 'pointer' : 'not-allowed'};`;
        els.pasteBtn.disabled = !canPaste;

        els.menu.style.cssText = `display: flex; left: ${Math.min(x + 10, window.innerWidth - 180)}px; top: ${Math.min(y + 10, window.innerHeight - 220)}px;`;
    }

    function clearEventSlots(eventId) {
        calendar.querySelectorAll(`.slot[data-event-id="${eventId}"]`).forEach(slot => {
            slot.classList.remove('selected'); slot.innerHTML = ''; slot.style.backgroundColor = ''; delete slot.dataset.eventId;
        });
    }

    function createEventFromSlots(slotList, commandArray, color) {
        const id = utils.generateEventId();
        state.eventDatabase[id] = { id, commands: commandArray, color, slots: slotList.map(slot => {
            slot.dataset.eventId = id; slot.classList.add('selected');
            return { day: Number(slot.dataset.day), time: slot.dataset.time };
        })};
        app.ui.renderEventUI(id);
        app.actions.syncEventToServer(id);
        return id;
    }

    Object.assign(app.actions, {
        hideMenu, showMenu, clearEventSlots, createEventFromSlots,

        deleteEvent: (eventId) => {
            if (!eventId || !state.eventDatabase[eventId]) return;
            clearEventSlots(eventId); delete state.eventDatabase[eventId];
            app.actions.deleteEventFromServer(eventId);
        },

        editEvent: (eventId) => {
            const evt = state.eventDatabase[eventId];
            if (!evt) return;
            app.ui.openModal('Edit Block', evt.commands.join('\n'), evt.color || '#007aff', (cmds, color) => {
                if (cmds === null) return;
                evt.commands = cmds.split('\n').map(c => c.trim()).filter(Boolean);
                evt.color = color;
                app.ui.renderEventUI(eventId);
                app.actions.syncEventToServer(eventId);
            }, true);
        },

        copyEventToClipboard: (eventId) => {
            const evt = state.eventDatabase[eventId];
            if (!evt || !evt.slots.length) return;
            const norm = evt.slots.map(s => ({ day: Number(s.day), row: utils.timeToRow(s.time) }));
            const minDay = Math.min(...norm.map(s => s.day)), minRow = Math.min(...norm.map(s => s.row));
            state.copiedBlockData = { commands: [...evt.commands], color: evt.color || '#007aff', offsets: norm.map(s => ({ d: s.day - minDay, r: s.row - minRow })) };
        },

        canPasteBlockAt: (targetSlot, clip = state.copiedBlockData) => {
            if (!targetSlot || !clip) return false;
            const bDay = Number(targetSlot.dataset.day), bRow = Number(targetSlot.dataset.row);
            return clip.offsets.every(off => {
                const slot = utils.getSlotByDayRow(bDay + off.d, bRow + off.r);
                return slot && !slot.dataset.eventId;
            });
        },

        pasteCopiedBlockAt: (targetSlot) => {
            if (!app.actions.canPasteBlockAt(targetSlot)) return false;
            const bDay = Number(targetSlot.dataset.day), bRow = Number(targetSlot.dataset.row);
            const slots = state.copiedBlockData.offsets.map(off => utils.getSlotByDayRow(bDay + off.d, bRow + off.r)).filter(Boolean);
            app.actions.createEventFromSlots(slots, [...state.copiedBlockData.commands], state.copiedBlockData.color);
        },

        syncEventToServer: async (eventId) => {
            const evt = state.eventDatabase[eventId];
            if (!evt || !evt.slots.length) return;
            const mins = evt.slots.map(s => { const [h, m] = s.time.split(':').map(Number); return h * 60 + m; });
            try {
                const res = await fetch('/schedule/set', {
                    method: 'POST', headers: { 'Content-Type': 'application/json' },
                    // String() wrapper removed, enforcing pure Number
                    body: JSON.stringify({ id: Number(eventId), commands: [evt.commands.map(c => `"${c}"`).join(' ')], color: evt.color, day: evt.slots[0].day, start_time: Math.min(...mins), end_time: Math.max(...mins) + 15 })
                });
                if (res.ok) window.location.reload();
            } catch (e) { console.error("Sync failed:", e); }
        },

        deleteEventFromServer: async (eventId) => {
            try {
                // String() wrapper removed, enforcing pure Number
                await fetch('/schedule/delete', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id: Number(eventId) })
                });
            }
            catch (e) { console.error("Delete failed:", e); }
        },

        fetchAndLoadSchedules: async () => {
            try {
                const res = await fetch('/schedule/json');
                const data = await res.json();
                Object.keys(state.eventDatabase).forEach(clearEventSlots);
                state.eventDatabase = {};

                for (const key in data) {
                    const evt = data[key];
                    // Enforce pure Number on incoming load
                    const numericId = Number(evt.id);
                    const slots = [], cmds = (evt.commands || []).flatMap(c => c.match(/"([^"]+)"/g)?.map(m => m.replace(/"/g, '')) || [c]);

                    for (let m = evt.start_time; m < evt.end_time; m += 15) {
                        const timeStr = `${Math.floor(m / 60)}:${(m % 60).toString().padStart(2, '0')}`;
                        slots.push({ day: evt.day, time: timeStr });
                        const domSlot = calendar.querySelector(`.slot[data-day="${evt.day}"][data-time="${timeStr}"]`);
                        if (domSlot) domSlot.dataset.eventId = numericId;
                    }
                    state.eventDatabase[numericId] = { id: numericId, commands: cmds, color: evt.color || '#33ff33', slots };
                    app.ui.renderEventUI(numericId);
                }
            } catch (e) { console.error("Load failed:", e); }
        },

        bindMenuActions: () => {
            if (app.actions._bound) return; app.actions._bound = true;
            document.addEventListener('pointerdown', (e) => {
                if (!e.target.closest('.context-menu, .modal-overlay, .slot.selected')) hideMenu();
            });
            els.editCmdBtn.onclick = () => { if (state.activeEventId) app.actions.editEvent(state.activeEventId); hideMenu(); };
            els.copyBtn.onclick = () => { if (state.activeEventId) app.actions.copyEventToClipboard(state.activeEventId); hideMenu(); };
            els.pasteBtn.onclick = () => { if (state.menuTargetSlot) app.actions.pasteCopiedBlockAt(state.menuTargetSlot); hideMenu(); };
            els.deleteBtn.onclick = () => { if (state.activeEventId) app.actions.deleteEvent(state.activeEventId); hideMenu(); };
        }
    });
})();
)rawliteral";
