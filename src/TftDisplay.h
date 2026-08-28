#pragma once

// Round GC9A01A TFT (SPI) - shows the force value in parallel to the OLED (display.h/.cpp)
void tftDisplaySetup();
void tftDisplayShowForce(float forceNewton);
void tftDisplayTest(); // fills the screen and prints a test message - call once after tftDisplaySetup() to verify wiring
