#pragma once

// Round GC9A01A TFT (SPI) - shows the force value in parallel to the OLED (display.h/.cpp)
void tftDisplaySetup();
// shows the numeric force value plus a line chart of the given history (oldest to newest,
// same array shape as Assembly.force.history / the web UI's forceHistory chart)
void tftDisplayShowForce(float forceNewton, const float *history, int historyCount);
// fills the screen with a startup banner (title + version/build info) - call once after
// tftDisplaySetup() to verify wiring
void tftDisplayTest(const char *versionInfo);
