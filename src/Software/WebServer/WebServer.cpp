// src/Software/WebServer/WebServer.cpp

#include "WebServer.h"
#include "../../SystemController/SystemController.h"
#include <cstdio>   // snprintf
#include <cstdlib>  // strtoul

// HTML in PROGMEM; <select> now holds numeric mode_id values
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"><title>LED Control</title>
  <style>button:disabled{opacity:.5;cursor:not-allowed}.control{margin-bottom:8px}</style>
</head>
<body>
  <h1>LED Strip Control</h1>
  <div class="control"><label>Color: <input type="color" id="color"/></label></div>
  <div class="control"><label>Brightness: <input type="range" id="brightness" min="0" max="255"/></label></div>
  <div class="control"><label>Mode:
    <select id="mode">
      <option value="0">Color Solid</option>
      <!-- more modes here with value="1", "2", ... -->
    </select>
  </label></div>
  <div class="control"><button id="btnOn">On</button> <button id="btnOff">Off</button></div>

  <script>
    function debounce(fn,d){let t;return(...a)=>{clearTimeout(t);t=setTimeout(()=>fn(...a),d);};}
    const ws=new WebSocket(`ws://${location.host}/ws`);
    ws.onmessage=evt=>{
      const msg=evt.data, tag=msg[0], data=msg.slice(1);
      switch(tag){
        case 'C': document.getElementById('color').value      = '#' + data;                     break;
        case 'B': document.getElementById('brightness').value = parseInt(data,10);             break;
        case 'S': updateButtons(data==='1');                                       break;
        case 'M': document.getElementById('mode').value       = data;                       break;
        case 'F': {
          const [hex,b,s,m]=data.split(',');
          document.getElementById('color').value      = '#'+hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s==='1');
          document.getElementById('mode').value       = m;
        } break;
      }
    };

    function send(k,v){ fetch(`/set?${k}=${encodeURIComponent(v)}`); }
    const sendColor      = debounce(()=>send('color',     document.getElementById('color').value),100);
    const sendBrightness = debounce(()=>send('brightness',document.getElementById('brightness').value),100);

    window.addEventListener('load',()=>{
      fetch('/state').then(r=>r.text()).then(msg=>{
        if(msg[0]==='F'){
          const [hex,b,s,m]=msg.slice(1).split(',');
          document.getElementById('color').value      = '#'+hex;
          document.getElementById('brightness').value = parseInt(b,10);
          updateButtons(s==='1');
          document.getElementById('mode').value       = m;
        }
      });
      document.getElementById('color')     .addEventListener('input', sendColor);
      document.getElementById('brightness').addEventListener('input', sendBrightness);
      document.getElementById('mode')      .addEventListener('change', ()=>send('mode_id', document.getElementById('mode').value));
      document.getElementById('btnOn')     .addEventListener('click', ()=>{ send('state','1'); updateButtons(true); });
      document.getElementById('btnOff')    .addEventListener('click', ()=>{ send('state','0'); updateButtons(false); });
    });

    function updateButtons(on){
      document.getElementById('btnOn').disabled  = on;
      document.getElementById('btnOff').disabled = !on;
    }
  </script>
</body>
</html>
)rawliteral";

WebServer::WebServer(SystemController& controller, AsyncWebServer& server)
  : controller_(controller), server_(server) {}

void WebServer::begin() {
  server_.on("/",      HTTP_GET, [this](auto* r){ serve_main_page(r); });
  server_.on("/set",   HTTP_GET, [this](auto* r){ handle_set(r); });
  server_.on("/state", HTTP_GET, [this](auto* r){ handle_get_state(r); });
  server_.addHandler(&ws_);
  ws_.onEvent([this](auto*, auto*, AwsEventType t, auto*, auto*, auto){
    if(t==WS_EVT_CONNECT) broadcast_led_state("full");
  });
  server_.begin();
}

void WebServer::serve_main_page(AsyncWebServerRequest* req) {
  req->send_P(200, "text/html", INDEX_HTML);
}

void WebServer::handle_set(AsyncWebServerRequest* req) {
  const char* field = nullptr;
  char buf[16];

  if (auto* p = req->getParam("color")) {
    // Convert "#RRGGBB" into "R G B"
    char hexbuf[8];
    p->value().toCharArray(hexbuf, sizeof(hexbuf));           // e.g. "#FF00FF"
    long v = strtol(hexbuf + 1, nullptr, 16);                 // parse hex
    snprintf(buf, sizeof(buf), "%u %u %u",
             (unsigned)((v >> 16) & 0xFF),
             (unsigned)((v >>  8) & 0xFF),
             (unsigned)(v & 0xFF));
    controller_.led_strip_set_rgb(buf);
    field = "color";
  }
  else if (auto* p = req->getParam("brightness")) {
    p->value().toCharArray(buf, sizeof(buf));                 // decimal string
    controller_.led_strip_set_brightness(buf);
    field = "brightness";
  }
  else if (auto* p = req->getParam("state")) {
    p->value().toCharArray(buf, sizeof(buf));                 // "0" or "1"
    controller_.led_strip_set_state(buf);
    field = "state";
  }
  else if (auto* p = req->getParam("mode_id")) {
    p->value().toCharArray(buf, sizeof(buf));                 // numeric ID
    controller_.led_strip_set_mode(buf);
    field = "mode";
  }

  if (field) broadcast_led_state(field);
  req->send(200, "text/plain", "ok");
}


void WebServer::handle_get_state(AsyncWebServerRequest* req) {
  update_state_payload("full");
  req->send(200, "text/plain", payload_);
}

void WebServer::update_state_payload(const char* field) {
  if      (strcmp(field,"color")==0) {
    String c = controller_.led_strip_get_color_hex();        // "#RRGGBB"
    unsigned long rgb = strtoul(c.c_str()+1, nullptr, 16);
    payload_len_ = snprintf(payload_, kBufSize,   "C%06lX", rgb);
  }
  else if (strcmp(field,"brightness")==0) {
    uint8_t b = controller_.led_strip_get_brightness();
    payload_len_ = snprintf(payload_, kBufSize,   "B%u", (unsigned)b);
  }
  else if (strcmp(field,"state")==0) {
    bool s = controller_.led_strip_get_state();
    payload_len_ = snprintf(payload_, kBufSize,   "S%u", (unsigned)s);
  }
  else if (strcmp(field,"mode")==0) {
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(payload_, kBufSize,   "M%u", (unsigned)m);
  }
  else {  // full
    String c = controller_.led_strip_get_color_hex();
    unsigned long rgb = strtoul(c.c_str()+1, nullptr, 16);
    uint8_t b = controller_.led_strip_get_brightness();
    bool    s = controller_.led_strip_get_state();
    uint8_t m = controller_.led_strip_get_mode_id();
    payload_len_ = snprintf(
      payload_,kBufSize,"F%06lX,%u,%u,%u",
      rgb,(unsigned)b,(unsigned)s,(unsigned)m
    );
  }

  if (payload_len_ >= kBufSize) payload_len_ = kBufSize - 1;
  payload_[payload_len_] = '\0';
}

void WebServer::broadcast_led_state(const char* field) {
  update_state_payload(field);
  ws_.textAll(payload_, payload_len_);
}
