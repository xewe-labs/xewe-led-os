# LedStrip

## Purpose
- allow the LED strip control in a way that is smooth and robust
- allow RGB, HSV control
- allow brightness control
- allow state control
- allow mode control


## Content
- LedStrip - main orchestrator for everything that affects the final LED strip state
- AsyncTimer - interface that allows to set the timer that runs in the background, with a start and end value mapped onto the timer progress
- Brightness - controls LED brightness and state
- LedMode -  controls the current led mode, from solid, to rainbow