#pragma once

#include <pgmspace.h>

static const char CALENDAR_HTML[] PROGMEM = R"rawliteral(
<!-- PASTE index.html CONTENT HERE -->
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Schedule</title>
  <link rel="stylesheet" href="{{ url_for('static', filename='style.css') }}">
</head>
<body>
  <div id="app"></div>
  <script src="static/calendar-core.js"></script>
  <script src="static/calendar-ui.js"></script>
  <script src="static/calendar-actions.js"></script>
  <script src="static/calendar-interactions.js"></script>
</body>
</html>
)rawliteral";
