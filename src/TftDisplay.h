#pragma once

// Round GC9A01A TFT (SPI) - shows the force value in parallel to the OLED (display.h/.cpp)
void tftDisplaySetup();
void tftDisplayShowForce(float forceNewton);
