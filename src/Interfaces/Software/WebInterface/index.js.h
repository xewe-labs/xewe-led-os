#pragma once

#include <pgmspace.h>

static const char WEB_INDEX_JS[] PROGMEM = R"rawliteral(
"use strict";

const DEBOUNCE_MS = 200;

const elements = {
  mode: document.getElementById('mode'),
  toggleAdditional: document.getElementById('toggleAdditional'),
  additionalToggleWrap: document.getElementById('additional-toggle-wrap'),
  resetDefaults: document.getElementById('resetDefaults'),
  resetDefaultsWrap: document.getElementById('reset-defaults-wrap'),
  btnOn: document.getElementById('btnOn'),
  btnOff: document.getElementById('btnOff'),
  statusIndicator: document.getElementById('status-indicator'),
  statusText: document.getElementById('status-text'),
  deviceTitle: document.getElementById('device-title'),
  sliderContainer: document.getElementById('slider-container'),
  brightness: document.getElementById('brightness'),
  brightnessValue: document.getElementById('brightnessValue')
};

let ws;
let reconnectTimer;
const STATE = { hue: 0, sat: 255, brightness: 128 };
let isOnline = false;
let reloadTimer = null;
let modesData = [];
let currentModeId = -1;
let showAdditionalParams = false;

async function loadName() {
  try {
    const res = await fetch('/name', { cache: 'no-store' });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    elements.deviceTitle.textContent = (await res.text()).trim() || 'LED Strip Control';
  } catch (e) {
    elements.deviceTitle.textContent = 'LED Strip Control';
  }
}

const HEARTBEAT_TIMEOUT_MS = 2200;
let lastHeartbeat = 0;

setInterval(() => {
  if (Date.now() - lastHeartbeat > HEARTBEAT_TIMEOUT_MS) setStatus(false);
}, 500);

const clamp255 = (x) => Math.max(0, Math.min(255, x | 0));

function hsvToRgb255(h255, s255, v255) {
  const h = ((h255 % 256) / 255) * 360;
  const s = clamp255(s255) / 255;
  const v = clamp255(v255) / 255;

  if (s <= 0) {
    const c = (v * 255) | 0;
    return [c, c, c];
  }

  const i = Math.floor(h / 60) % 6;
  const f = h / 60 - Math.floor(h / 60);
  const p = v * (1 - s);
  const q = v * (1 - f * s);
  const t = v * (1 - (1 - f) * s);

  let r, g, b;
  switch (i) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }

  return [
    clamp255(Math.round(r * 255)),
    clamp255(Math.round(g * 255)),
    clamp255(Math.round(b * 255))
  ];
}

function rgbToHsv255(r, g, b) {
  const rf = r / 255;
  const gf = g / 255;
  const bf = b / 255;
  const max = Math.max(rf, gf, bf);
  const min = Math.min(rf, gf, bf);
  const d = max - min;

  let h = 0;
  let s = max === 0 ? 0 : d / max;
  const v = max;

  if (d !== 0) {
    switch (max) {
      case rf: h = ((gf - bf) / d + (gf < bf ? 6 : 0)); break;
      case gf: h = ((bf - rf) / d + 2); break;
      default: h = ((rf - gf) / d + 4); break;
    }
    h *= 60;
  }

  return [
    clamp255(Math.round(h / 360 * 255)),
    clamp255(Math.round(s * 255)),
    clamp255(Math.round(v * 255))
  ];
}

const rgbToHex = (r, g, b) =>
  [r, g, b].map(x => x.toString(16).padStart(2, "0")).join("").toUpperCase();

const hexToRgb = (hex) => [
  parseInt(hex.slice(0, 2), 16),
  parseInt(hex.slice(2, 4), 16),
  parseInt(hex.slice(4, 6), 16)
];

const setStatus = (online) => {
  if (isOnline !== online) {
    if (!online) {
      if (!reloadTimer) reloadTimer = setTimeout(() => location.reload(), 1000);
    } else {
      if (reloadTimer) {
        clearTimeout(reloadTimer);
        reloadTimer = null;
      }
    }
    isOnline = online;
  }

  elements.statusIndicator.style.background = online ? 'var(--green)' : 'var(--red)';
  elements.statusText.textContent = online ? 'Online' : 'Offline';
};

const updateButtons = (isOn) => {
  elements.btnOn.disabled = isOn;
  elements.btnOff.disabled = !isOn;
};

const debounce = (fn, d) => {
  let t;
  return (...a) => {
    clearTimeout(t);
    t = setTimeout(() => fn(...a), d);
  };
};

function isPrimaryHueParam(paramOrKey) {
  const key = typeof paramOrKey === 'string'
    ? paramOrKey
    : (paramOrKey?.key ?? '');
  const k = String(key).toLowerCase();
  return k === 'hue' || k === 'h';
}

function isPrimarySatParam(paramOrKey) {
  const key = typeof paramOrKey === 'string'
    ? paramOrKey
    : (paramOrKey?.key ?? '');
  const k = String(key).toLowerCase();
  return k === 'sat' || k === 's';
}

function isHueStyledParam(param) {
  const key = String(param?.key ?? '').toLowerCase();
  const name = String(param?.display_name ?? param?.name ?? '').toLowerCase();
  return isPrimaryHueParam(param) || key.includes('hue') || name.includes('hue');
}

function isSatStyledParam(param) {
  const key = String(param?.key ?? '').toLowerCase();
  const name = String(param?.display_name ?? param?.name ?? '').toLowerCase();
  return (
    isPrimarySatParam(param) ||
    key.includes('sat') ||
    name.includes('sat') ||
    key.includes('saturation') ||
    name.includes('saturation')
  );
}

function toByteFromRangeValue(rawValue, minValue, maxValue) {
  const raw = Number(rawValue);
  const min = Number(minValue);
  const max = Number(maxValue);

  if (!Number.isFinite(raw)) return 0;
  if (!Number.isFinite(min) || !Number.isFinite(max) || max <= min) {
    return clamp255(Math.round(raw));
  }

  const t = (raw - min) / (max - min);
  return clamp255(Math.round(t * 255));
}

function inputValueToByte(input) {
  if (!input) return 0;
  return toByteFromRangeValue(input.value, input.min, input.max);
}

function paramValueToByte(param, rawValue) {
  return toByteFromRangeValue(rawValue, getParamMin(param), getParamMax(param));
}

function updateVisuals() {
  const fallbackHueEl =
    document.getElementById('param_hue') ||
    document.getElementById('param_h') ||
    document.querySelector('input.hue');

  const fallbackSatEl =
    document.getElementById('param_sat') ||
    document.getElementById('param_s') ||
    document.querySelector('input.sat');

  const h = fallbackHueEl ? inputValueToByte(fallbackHueEl) : STATE.hue;
  const s = fallbackSatEl ? inputValueToByte(fallbackSatEl) : STATE.sat;
  const v = STATE.brightness;

  const hueEls = Array.from(document.querySelectorAll('input.hue'));
  const satEls = Array.from(document.querySelectorAll('input.sat'));
  const elBri = elements.brightness;

  satEls.forEach(el => {
    const satVal = inputValueToByte(el);
    const [rF, gF, bF] = hsvToRgb255(h, 255, 255);
    el.style.setProperty(
      "--track-bg",
      `linear-gradient(to right, #ffffff, rgb(${rF}, ${gF}, ${bF}))`
    );

    const [rT, gT, bT] = hsvToRgb255(h, satVal, 255);
    el.style.setProperty(
      "--thumb-bg",
      `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${rT}, ${gT}, ${bT})`
    );
  });

  hueEls.forEach(el => {
    const hueVal = inputValueToByte(el);
    const [rT, gT, bT] = hsvToRgb255(hueVal, s, 255);
    el.style.setProperty(
      "--thumb-bg",
      `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${rT}, ${gT}, ${bT})`
    );
  });

  if (hueEls.length || satEls.length) {
    const [r0, g0, b0] = hsvToRgb255(h, s, 8);
    const [r1, g1, b1] = hsvToRgb255(h, s, 255);
    elBri.style.setProperty(
      "--track-bg",
      `linear-gradient(to right, rgb(${r0}, ${g0}, ${b0}), rgb(${r1}, ${g1}, ${b1}))`
    );

    const [rB, gB, bB] = hsvToRgb255(h, s, v);
    elBri.style.setProperty(
      "--thumb-bg",
      `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${rB}, ${gB}, ${bB})`
    );
  } else {
    elBri.style.setProperty(
      "--track-bg",
      `linear-gradient(to right, rgb(20, 20, 20), rgb(255, 255, 255))`
    );
    elBri.style.setProperty(
      "--thumb-bg",
      `radial-gradient(circle at 35% 35%, rgba(255,255,255,.9), rgba(255,255,255,.1)), rgb(${v}, ${v}, ${v})`
    );
  }
}

function getParamKind(param) {
  if (param && (param.type === 'a' || param.type === 'b')) return param.type;

  const values = Object.values(param || {});
  const lastValue = values.length ? values[values.length - 1] : 'b';
  return (lastValue === 'a' || lastValue === 'b') ? lastValue : 'b';
}

function isAdditionalParam(param) {
  return getParamKind(param) === 'a';
}

function getParamMin(param) {
  return param.min !== undefined ? param.min : param.min_value;
}

function getParamMax(param) {
  return param.max !== undefined ? param.max : param.max_value;
}

function getParamStep(param) {
  return param.step !== undefined ? param.step : param.step_value;
}

function getParamValue(param) {
  if (param.value !== undefined) return param.value;
  if (param.default_value !== undefined) return param.default_value;
  return 0;
}

function setModeParamValue(modeId, key, val) {
  const mode = modesData.find(m => m.id == modeId);
  if (!mode || !Array.isArray(mode.params)) return;

  const param = mode.params.find(p => p.key === key);
  if (!param) return;

  if (param.value !== undefined) param.value = val;
  if (param.default_value !== undefined) param.default_value = val;
}

function updateParamUI(key, val) {
  const aliases = [key];

  if (key === 'hue') aliases.push('h');
  else if (key === 'h') aliases.push('hue');
  else if (key === 'sat') aliases.push('s');
  else if (key === 's') aliases.push('sat');

  aliases.forEach(k => {
    setModeParamValue(currentModeId, k, val);

    const el = document.getElementById(`param_${k}`);
    const out = document.getElementById(`val_${k}`);

    if (el) el.value = val;
    if (out) out.value = val;
  });
}

function updateAdditionalToggle(mode) {
  if (!mode || !Array.isArray(mode.params)) {
    elements.additionalToggleWrap.style.display = 'none';
    return;
  }

  const hasAdditional = mode.params.some(p =>
    p.key !== 'brightness' &&
    p.key !== 'v' &&
    isAdditionalParam(p)
  );

  elements.additionalToggleWrap.style.display = hasAdditional ? '' : 'none';
  elements.toggleAdditional.textContent = showAdditionalParams
    ? 'Hide Additional Parameters'
    : 'Show Additional Parameters';
}

function updateResetDefaultsButton(mode) {
  if (!mode || !Array.isArray(mode.params)) {
    elements.resetDefaultsWrap.style.display = 'none';
    return;
  }

  const hasResettableParams = mode.params.some(p =>
    p.key !== 'brightness' &&
    p.key !== 'v'
  );

  elements.resetDefaultsWrap.style.display = hasResettableParams ? '' : 'none';
}

const sendCommand = (k, v) =>
  fetch(`/set?${k}=${encodeURIComponent(v)}`).catch(err => console.error(err));

const sendParam = debounce((k, v) =>
  fetch(`/set?param=${encodeURIComponent(k)}&val=${encodeURIComponent(v)}`).catch(e => console.error(e)),
  DEBOUNCE_MS
);

const sendColor = debounce(() => {
  const [r, g, b] = hsvToRgb255(STATE.hue, STATE.sat, 255);
  sendCommand('color', rgbToHex(r, g, b));
}, DEBOUNCE_MS);

const sendBrightness = debounce(() =>
  sendCommand('brightness', STATE.brightness),
  DEBOUNCE_MS
);

function renderParams(modeId) {
  const mode = modesData.find(m => m.id == modeId);
  elements.sliderContainer.innerHTML = '';

  if (!mode || !Array.isArray(mode.params)) {
    updateAdditionalToggle(mode);
    updateResetDefaultsButton(mode);
    updateVisuals();
    return;
  }

  updateAdditionalToggle(mode);
  updateResetDefaultsButton(mode);

  const visibleParams = mode.params.filter(p => {
    if (p.key === 'brightness' || p.key === 'v') return false;
    if (!showAdditionalParams && isAdditionalParam(p)) return false;
    return true;
  });

  visibleParams.forEach(p => {
    const isPrimaryH = isPrimaryHueParam(p);
    const isPrimaryS = isPrimarySatParam(p);
    const isHueStyled = isHueStyledParam(p);
    const isSatStyled = isSatStyledParam(p);

    let cls = 'generic';
    let val = Number(getParamValue(p));
    if (!Number.isFinite(val)) val = 0;

    if (isHueStyled) {
      cls = 'hue';
      if (isPrimaryH) STATE.hue = paramValueToByte(p, val);
    } else if (isSatStyled) {
      cls = 'sat';
      if (isPrimaryS) STATE.sat = paramValueToByte(p, val);
    }

    const wrap = document.createElement('div');
    wrap.className = 'control';
    wrap.innerHTML = `
      <div class="range-wrap">
        <input
          type="range"
          id="param_${p.key}"
          class="range ${cls}"
          min="${getParamMin(p)}"
          max="${getParamMax(p)}"
          step="${getParamStep(p)}"
          aria-label="${p.display_name}"
          value="${val}"
        />
        <output id="val_${p.key}" class="bubble">${val}</output>
      </div>
      <span class="param-label">${p.display_name}</span>
    `;
    elements.sliderContainer.appendChild(wrap);

    const input = wrap.querySelector('input');
    const output = wrap.querySelector('output');

    input.addEventListener('input', () => {
      const nextVal = Number(input.value);
      const safeVal = Number.isFinite(nextVal) ? nextVal : 0;

      output.value = input.value;
      setModeParamValue(currentModeId, p.key, safeVal);

      if (isPrimaryH) {
        STATE.hue = inputValueToByte(input);
        updateVisuals();
        sendColor();
      } else if (isPrimaryS) {
        STATE.sat = inputValueToByte(input);
        updateVisuals();
        sendColor();
      } else {
        if (isHueStyled || isSatStyled) updateVisuals();
        sendParam(p.key, input.value);
      }
    });
  });

  updateVisuals();
}

async function loadModes() {
  try {
    const res = await fetch('/modes', { cache: 'no-store' });
    modesData = await res.json();

    if (elements.mode.options.length <= 1) {
      elements.mode.innerHTML = '';
      modesData.forEach(m => {
        const opt = document.createElement('option');
        opt.value = m.id;
        opt.textContent = m.name;
        elements.mode.appendChild(opt);
      });
    }

    if (currentModeId === -1 && modesData.length > 0) currentModeId = modesData[0].id;
    elements.mode.value = currentModeId;
    renderParams(currentModeId);
  } catch (e) {
    console.error(e);
  }
}

function handleWireMessage(raw) {
  const tag = raw[0];
  const data = raw.slice(1);

  if (tag === 'H') {
    lastHeartbeat = Date.now();
    setStatus(true);
    return;
  }

  switch (tag) {
    case 'C': {
      const [r, g, b] = hexToRgb(data);
      let [h, s] = rgbToHsv255(r, g, b);

      if (h === 0 && STATE.hue === 255) h = 255;
      if (s > 0) STATE.hue = h;
      STATE.sat = s;

      updateParamUI('hue', STATE.hue);
      updateParamUI('sat', STATE.sat);
      updateVisuals();
      break;
    }

    case 'B': {
      STATE.brightness = clamp255(parseInt(data, 10) || 0);
      elements.brightness.value = STATE.brightness;
      elements.brightnessValue.value = STATE.brightness;
      updateVisuals();
      break;
    }

    case 'S':
      updateButtons(data === '1');
      break;

    case 'M':
      if (currentModeId !== parseInt(data, 10)) {
        currentModeId = parseInt(data, 10);
        showAdditionalParams = false;
        elements.mode.value = currentModeId;
        loadModes();
      }
      break;

    case 'P': {
      const splitIdx = data.indexOf(':');
      if (splitIdx > 0) {
        const key = data.slice(0, splitIdx);
        const val = parseInt(data.slice(splitIdx + 1), 10);

        updateParamUI(key, val);

        if (key === 'hue' || key === 'h') {
          STATE.hue = val;
          updateVisuals();
        } else if (key === 'sat' || key === 's') {
          STATE.sat = val;
          updateVisuals();
        }
      }
      break;
    }

    case 'F': {
      const [hex, bStr, sStr, mStr] = data.split(',');
      const [r, g, bb] = hexToRgb(hex);
      let [h, s] = rgbToHsv255(r, g, bb);

      if (h === 0 && STATE.hue === 255) h = 255;
      if (s > 0) STATE.hue = h;
      STATE.sat = s;
      STATE.brightness = clamp255(parseInt(bStr, 10) || 0);

      let modeChanged = false;
      if (currentModeId !== parseInt(mStr, 10)) {
        currentModeId = parseInt(mStr, 10);
        showAdditionalParams = false;
        elements.mode.value = currentModeId;
        modeChanged = true;
      }

      updateParamUI('hue', STATE.hue);
      updateParamUI('sat', STATE.sat);

      elements.brightness.value = STATE.brightness;
      elements.brightnessValue.value = STATE.brightness;

      updateButtons(sStr === '1');

      if (modeChanged) {
        loadModes();
      } else {
        updateVisuals();
      }
      break;
    }
  }
}

async function refreshState() {
  try {
    const res = await fetch('/state', { cache: 'no-store' });
    if (!res.ok) return;
    const text = (await res.text()).trim();
    if (text) handleWireMessage(text);
  } catch (e) {
    console.error(e);
  }
}

function connect() {
  if (ws && (ws.readyState === ws.CONNECTING || ws.readyState === ws.OPEN)) return;

  ws = new WebSocket(`ws://${location.hostname}:81/`);

  ws.onopen = () => {
    lastHeartbeat = Date.now();
    setStatus(true);
  };

  ws.onclose = () => {
    setStatus(false);
    clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connect, 5000);
  };

  ws.onerror = () => {
    try { ws.close(); } catch (e) {}
  };

  ws.onmessage = (e) => {
    handleWireMessage(e.data);
  };
}

window.addEventListener('load', () => {
  elements.btnOn.addEventListener('click', () => {
    sendCommand('state', '1');
    updateButtons(true);
  });

  elements.btnOff.addEventListener('click', () => {
    sendCommand('state', '0');
    updateButtons(false);
  });

  elements.toggleAdditional.addEventListener('click', () => {
    showAdditionalParams = !showAdditionalParams;
    renderParams(currentModeId);
  });

  elements.resetDefaults.addEventListener('click', async () => {
    await sendCommand('reset_params', '1');
    await refreshState();
    await loadModes();
  });

  elements.mode.addEventListener('change', async () => {
    currentModeId = parseInt(elements.mode.value, 10);
    showAdditionalParams = false;
    await sendCommand('mode_id', currentModeId);
    await loadModes();
  });

  elements.brightness.addEventListener('input', () => {
    STATE.brightness = clamp255(parseInt(elements.brightness.value, 10) || 0);
    elements.brightnessValue.value = STATE.brightness;
    updateVisuals();
    sendBrightness();
  });

  loadName();
  loadModes();
  connect();
});
)rawliteral";