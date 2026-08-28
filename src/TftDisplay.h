#pragma once

// Round GC9A01A TFT (SPI) - shows the force value in parallel to the OLED (display.h/.cpp)
void tftDisplaySetup();
// shows the numeric force value plus a line chart of the given history (oldest to newest,
// same array shape as Assembly.force.history / the web UI's forceHistory chart); ipText is
// shown small at the bottom, same as the "#Force" line on the OLED (display.h/.cpp)
void tftDisplayShowForce(float forceNewton, const float *history, int historyCount, const char *ipText);
// fills the screen with a startup banner (title + version/build info) - call once after
// tftDisplaySetup() to verify wiring
void tftDisplayTest(const char *versionInfo);
