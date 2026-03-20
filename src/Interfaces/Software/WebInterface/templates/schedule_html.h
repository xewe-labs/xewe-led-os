#pragma once
#include <pgmspace.h>
static const char SCHEDULE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>XeWe LED: Scheduler</title>
    <link rel="stylesheet" href="/static/schedule-style.css">
</head>
<body>

    <div id="app">
        <div style="margin-bottom: 1rem; text-align: left;">
            <button type="button" class="secondary" onclick="window.location.href='/'">&#8592; Back to Home</button>
        </div>

        <h2>LED Scheduler</h2>
        <div class="calendar-container" id="calendar">
            </div>
    </div>

    <script src="static/schedule-utils.js"></script>
    <script src="static/schedule-core.js"></script>
    <script src="static/schedule-ui.js"></script>
    <script src="static/schedule-actions.js"></script>
    <script src="static/schedule-interactions.js"></script>

</body>
</html>
)rawliteral";