#pragma once
#include <pgmspace.h>
static const char SCHEDULE_UI_JS[] PROGMEM = R"rawliteral(
// calendar-ui.js
(function () {
    const app = window.CalendarApp;
    const { calendar, config, state, els, utils } = app;

    function buildUI() {
        if (els.calendarShell) return;

        // 1. Build Shell, Time Rail, Modal, and Menu in one shot
        calendar.insertAdjacentHTML('beforebegin', `<div class="calendar-shell"><div class="calendar-time-rail"><div class="calendar-time-rail-inner"></div></div></div>`);
        calendar.insertAdjacentHTML('beforeend', `<div class="now-line" style="display:none;"></div>`);

        document.body.insertAdjacentHTML('beforeend', `
            <div class="modal-overlay" style="display:none;">
                <div class="modal-box">
                    <h3 class="modal-title"></h3>
                    <textarea class="modal-textarea" rows="6"></textarea>
                    <div class="color-container"><span class="color-label">Color:</span><input type="color" class="modal-color-input"></div>
                    <div class="modal-btn-container"><button class="btn-cancel">Cancel</button><button class="btn-save">Save</button></div>
                </div>
            </div>
            <div class="context-menu" style="display:none;">
                <button class="menu-btn" id="m-edit">Edit Commands / Color</button>
                <button class="menu-btn" id="m-copy">Copy Block</button>
                <button class="menu-btn" id="m-paste">Paste Block</button>
                <button class="menu-btn menu-btn-delete" id="m-del">Delete Block</button>
            </div>
            <style id="calendar-dynamic-style"></style>
        `);

        // 2. Map Elements
        Object.assign(els, {
            calendarShell: calendar.previousElementSibling,
            timeRailInner: calendar.previousElementSibling.querySelector('.calendar-time-rail-inner'),
            nowLine: calendar.querySelector('.now-line'),
            modalOverlay: document.querySelector('.modal-overlay'),
            modalBox: document.querySelector('.modal-box'),
            modalTitle: document.querySelector('.modal-title'),
            modalTextarea: document.querySelector('.modal-textarea'),
            modalColorInput: document.querySelector('.modal-color-input'),
            cancelModalBtn: document.querySelector('.btn-cancel'),
            saveModalBtn: document.querySelector('.btn-save'),
            menu: document.querySelector('.context-menu'),
            editCmdBtn: document.getElementById('m-edit'),
            copyBtn: document.getElementById('m-copy'),
            pasteBtn: document.getElementById('m-paste'),
            deleteBtn: document.getElementById('m-del'),
            dynamicStyle: document.getElementById('calendar-dynamic-style')
        });

        els.calendarShell.appendChild(calendar); // Move calendar into shell

        // 3. Modal Events (Preserving editing flag auto-color logic)
        els.modalTextarea.addEventListener('input', () => {
            if (!state.isEditingModal) {
                const detected = utils.detectColorFromCommands(els.modalTextarea.value);
                if (detected) els.modalColorInput.value = detected;
            }
        });

        els.cancelModalBtn.onclick = () => { if (state.modalCallback) state.modalCallback(null, null); closeModal(); };
        els.saveModalBtn.onclick = () => { if (state.modalCallback) state.modalCallback(els.modalTextarea.value, els.modalColorInput.value); closeModal(); };
    }

    function buildGridAndTimeRail() {
        // Time Rail
        let railHtml = '<div class="calendar-time-rail-spacer"></div>';
        for (let h = config.startHour; h < config.endHour; h++) {
            for (let q = 0; q < 4; q++) {
                railHtml += `<div class="calendar-time-rail-row ${q === 0 ? 'hour-marker' : ''}">${q === 0 ? `<span>${h % 12 || 12} ${h >= 12 ? 'PM' : 'AM'}</span>` : ''}</div>`;
            }
        }
        els.timeRailInner.innerHTML = railHtml + '<div class="calendar-time-rail-row hour-marker end-marker"><span>12 AM</span></div>';

        // Grid
        const today = utils.getTodayIndex();
        let gridHtml = config.days.map((d, i) => `<div class="day-header ${i === today ? 'today' : ''}"><div class="day-name">${d}</div></div>`).join('');

        for (let h = config.startHour; h < config.endHour; h++) {
            for (let q = 0; q < 4; q++) {
                const time = `${h}:${q === 0 ? '00' : q * 15}`;
                const row = (h - config.startHour) * 4 + q;
                for (let d = 0; d < 7; d++) {
                    gridHtml += `<div class="slot ${q === 0 ? 'hour-marker' : ''}" data-day="${d}" data-time="${time}" data-row="${row}"></div>`;
                }
            }
        }
        calendar.insertAdjacentHTML('beforeend', gridHtml + Array(7).fill('<div class="slot hour-marker end-marker"></div>').join(''));
    }

    function applyFixedScale() {
        const qtrPx = config.pixelsPerHour / 4;
        calendar.style.gridTemplateColumns = `repeat(7, minmax(80px, 1fr))`;
        calendar.style.gridTemplateRows = `var(--calendar-header-height, 60px) repeat(${utils.getTotalRows()}, ${qtrPx}px) 0px`;

        els.dynamicStyle.textContent = `
            .slot, .calendar-time-rail-row { height: ${qtrPx}px !important; min-height: ${qtrPx}px !important; max-height: ${qtrPx}px !important; }
            .calendar-time-rail-spacer { height: var(--calendar-header-height, 60px) !important; min-height: var(--calendar-header-height, 60px) !important; }
            .calendar-time-rail-row.end-marker { height: 0px !important; min-height: 0px !important; }
            body.calendar-dragging, body.calendar-dragging * { user-select: none !important; -webkit-user-select: none !important; cursor: crosshair !important; }
        `;
        syncTimeRailScroll(); updateNowIndicator();
    }

    function openModal(title, text, color, callback, isEditing = false) {
        state.isEditingModal = isEditing;
        els.modalTitle.textContent = title;
        els.modalTextarea.value = text;
        els.modalColorInput.value = (!isEditing && utils.detectColorFromCommands(text)) || color;
        state.modalCallback = callback;
        els.modalOverlay.style.display = 'flex';
        els.modalTextarea.focus();
    }

    function closeModal() { els.modalOverlay.style.display = 'none'; state.modalCallback = null; }
    function syncTimeRailScroll() { if (els.timeRailInner) els.timeRailInner.style.transform = `translateY(${-calendar.scrollTop}px)`; }

    function updateNowIndicator() {
        const header = calendar.querySelectorAll('.day-header')[utils.getTodayIndex()];
        const now = new Date();
        const hrs = now.getHours();

        if (!header || hrs < config.startHour || hrs >= config.endHour) return els.nowLine.style.display = 'none';

        els.nowLine.style.cssText = `display: block; left: ${header.offsetLeft}px; top: ${utils.getCalendarHeaderHeightPx() + ((hrs - config.startHour) + (now.getMinutes() / 60)) * config.pixelsPerHour}px; width: ${header.offsetWidth}px;`;
    }

    function renderEventUI(eventId) {
        const event = state.eventDatabase[eventId];
        if (!event) return;

        calendar.querySelectorAll(`.slot[data-event-id="${eventId}"]`).forEach(s => {
            s.innerHTML = ''; s.style.backgroundColor = event.color || '#007aff'; s.classList.add('selected');
        });

        const sortedSlots = event.slots.map(s => calendar.querySelector(`.slot[data-day="${s.day}"][data-time="${s.time}"]`)).filter(Boolean)
            .sort((a, b) => a.dataset.row - b.dataset.row);

        if (sortedSlots.length) {
            sortedSlots[0].style.position = 'relative';
            sortedSlots[0].innerHTML = `<div class="slot-text" style="height: ${sortedSlots.length * 100}%;">${event.commands.map(cmd => `<div style="white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">> ${cmd}</div>`).join('')}</div>`;
        }
    }

    Object.assign(app.ui, {
        init: () => {
            if (app.ui._initialized) return; app.ui._initialized = true;
            buildUI(); buildGridAndTimeRail(); applyFixedScale();
            calendar.addEventListener('scroll', syncTimeRailScroll);
            updateNowIndicator(); setInterval(updateNowIndicator, 30000);
        },
        openModal, closeModal, updateNowIndicator, applyFixedScale, renderEventUI, syncTimeRailScroll
    });
})();
)rawliteral";
