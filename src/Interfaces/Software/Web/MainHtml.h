#pragma once
// Matches your working markup (labels removed). Adds robust SSE re-connect handling:
// - marks online on EventSource `open`
// - sends immediate state fetch on reconnect
// - keeps "hb" heartbeat handling
// - one-time fallback auto-reload if offline persists > ~7s
static const char* INDEX_HTML = R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>XeWe LED</title>
  <link rel="stylesheet" href="/static/styles.css">
</head>
<body>

  <header class="page-header">
    <div class="wrap">
      <h1 class="page-title">XeWe LED</h1>
    </div>
  </header>

  <main class="wrap">

    <section class="block" aria-label="{{ name }} controls">
      <h2 class="block-title">
        <span id="device-name">{{ name }}</span>
        <span id="device-status" class="status" role="status" aria-live="polite" data-online="false">
          <span class="dot" aria-hidden="true"></span>
          <span class="sr-only">Offline</span>
        </span>
      </h2>

      <!-- 1) Color slider -->
      <div class="row">
        <input id="color" type="range" min="0" max="360" value="{{ state.hue }}" aria-label="Color">
      </div>

      <!-- 2) Brightness slider -->
      <div class="row">
        <input id="brightness" type="range" min="0" max="100" value="{{ state.brightness }}" aria-label="Brightness">
      </div>

      <!-- 3) Power radio-style switch -->
      <div class="row">
        <div class="seg" role="radiogroup" aria-label="Power">
          <button id="on"  type="button" role="radio" aria-checked="false">On</button>
          <button id="off" type="button" role="radio" aria-checked="true">Off</button>
        </div>
      </div>

      <!-- 4) Mode dropdown -->
      <div class="row">
        <select id="mode" aria-label="Mode">
          <option value="solid">Solid</option>
          <option value="perlin-noise">Perlin Noise</option>
          <option value="rainbow">Rainbow</option>
        </select>
      </div>

      <!-- 5) Advanced button row -->
      <div class="row row--full">
        <a class="btn" href="/advanced" role="button" aria-label="Open advanced controls">Advanced</a>
      </div>
    </section>

  </main>

  <script>
    const color = document.getElementById('color');
    const bri   = document.getElementById('brightness');
    const btnOn = document.getElementById('on');
    const btnOff= document.getElementById('off');
    const mode  = document.getElementById('mode');
    const nameEl= document.getElementById('device-name');
    const statusEl = document.getElementById('device-status');
    mode.value  = "{{ state.mode }}";

    function post(patch){
      fetch('/api/state', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify(patch)
      }).catch(()=>{});
    }

    // Client-side conversions
    function hsvToRgbDeg(h, s, v){
      h = ((h % 360) + 360) % 360;
      const hf = h/60, i=Math.floor(hf), f=hf-i;
      const sf=s/255, vf=v/255;
      const p=vf*(1-sf), q=vf*(1-sf*f), t=vf*(1-sf*(1-f));
      let r=0,g=0,b=0;
      switch(i){case 0:r=vf;g=t;b=p;break;case 1:r=q;g=vf;b=p;break;case 2:r=p;g=vf;b=t;break;
                 case 3:r=p;g=q;b=vf;break;case 4:r=t;g=p;b=vf;break;default:r=vf;g=p;b=q;}
      return [Math.round(r*255),Math.round(g*255),Math.round(b*255)];
    }
    function rgbToHueDeg(r,g,b){
      const rf=r/255,gf=g/255,bf=b/255, mx=Math.max(rf,gf,bf), mn=Math.min(rf,gf,bf), d=mx-mn;
      if(!d) return 0; let h = mx===rf? ((gf-bf)/d)%6 : mx===gf? (bf-rf)/d+2 : (rf-gf)/d+4;
      h*=60; if(h<0) h+=360; return Math.round(h);
    }
    const bri255ToPercent = b => Math.round(b*100/255);
    const percentToBri255 = p => Math.round(p*255/100);

    // Brightness: black -> dim knee -> full color
    function briGradient(h){
      return `linear-gradient(90deg,
        #000 0%,
        hsl(${h} 100% 10%) 12%,
        hsl(${h} 100% 50%) 100%)`;
    }
    function updateColorUI(){
      const h = +color.value;
      bri.style.setProperty('--h', h);
      bri.style.background = briGradient(h);
    }

    // Power UI lock
    function setPowerUI(isOn){
      btnOn.setAttribute('aria-checked', isOn ? 'true' : 'false');
      btnOff.setAttribute('aria-checked', !isOn ? 'true' : 'false');
      btnOn.disabled  = isOn;
      btnOff.disabled = !isOn;
    }

    // Online indicator + accessible text
    function setOnlineUI(isOnline){
      statusEl.dataset.online = isOnline ? 'true' : 'false';
      const sr = statusEl.querySelector('.sr-only');
      if (sr) sr.textContent = isOnline ? 'Online' : 'Offline';
      statusEl.setAttribute('aria-label', isOnline ? 'Device is online' : 'Device is offline');
    }

    // Send user changes (canonical payload expected by backend)
    color.addEventListener('input', updateColorUI);
    color.addEventListener('change', ()=> post({rgb: hsvToRgbDeg(+color.value, 255, 255)}));
    bri  .addEventListener('change', ()=> post({brightness: percentToBri255(+bri.value)}));

    btnOn.addEventListener('click', ()=>{
      if (btnOn.disabled) return;
      setPowerUI(true);
      post({power:true});
    });
    btnOff.addEventListener('click', ()=>{
      if (btnOff.disabled) return;
      setPowerUI(false);
      post({power:false});
    });

    mode.addEventListener('change', ()=> post({mode: mode.value}));

    // Realtime patches (SSE) with robust reconnect
    function applyPatch(obj){
      if (obj.rgb){ const [r,g,b]=obj.rgb; color.value = rgbToHueDeg(r,g,b); updateColorUI(); }
      if (obj.brightness!==undefined){ bri.value = bri255ToPercent(+obj.brightness); }
      if (obj.power!==undefined){ setPowerUI(!!obj.power); }
      if (obj.mode!==undefined){ mode.value = obj.mode; }
      if (obj.name){ nameEl.textContent = obj.name; }
      if (obj.online!==undefined){ setOnlineUI(!!obj.online); }
    }

    let lastHb = 0;
    let wasOnline = false;
    let didReload = false;

    try{
      const es = new EventSource('/events');

      // When the SSE connection opens (e.g., board rebooted and came back)
      es.onopen = () => {
        lastHb = Date.now();
        wasOnline = true;
        setOnlineUI(true);
        // Pull full state to resync UI immediately after reconnect
        fetch('/api/state').then(r=>r.ok?r.json():null).then(s=>{ if(s) applyPatch(s); }).catch(()=>{});
      };

      es.onerror = () => {
        // Connection problem; the interval below will mark offline if it persists.
      };

      es.addEventListener('state', ev=>{
        try{ applyPatch(JSON.parse(ev.data)); lastHb = Date.now(); wasOnline = true; }catch(e){}
      });
      es.addEventListener('patch', ev=>{
        try{ applyPatch(JSON.parse(ev.data)); lastHb = Date.now(); wasOnline = true; }catch(e){}
      });
      es.addEventListener('hb', ev=>{
        lastHb = Date.now();
        wasOnline = true;
        setOnlineUI(true);
      });
    }catch(e){}

    // Offline decay + one-time auto-refresh fallback
    setInterval(()=>{
      const offline = (Date.now() - lastHb) > 5000;
      if (offline) {
        setOnlineUI(false);
        if (wasOnline && !didReload) {
          didReload = true;
          // Give it a bit more time; if still offline, reload once
          setTimeout(()=>{ if ((Date.now() - lastHb) > 7000) location.reload(); }, 1500);
        }
      }
    }, 1000);

    // Init from server-rendered placeholders
    updateColorUI();
  </script>
</body>
</html>)HTML";
