#include "TftDisplay.h"

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#include "HwInterface.h"

namespace
{
    constexpr int16_t DISPLAY_SIZE = 240; // GC9A01A round display, 240x240

    SPIClass tftSpi(FSPI);
    Adafruit_GC9A01A tft(&tftSpi, TFT_DC, TFT_CS, TFT_RST);

    float lastShownForce = NAN;
}

void tftDisplaySetup()
{
    tftSpi.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(GC9A01A_BLACK);
}

void tftDisplayShowForce(float forceNewton)
{
    if (forceNewton == lastShownForce)
    {
        return; // unchanged since last draw, skip flicker
    }
    lastShownForce = forceNewton;

    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(4);

    char text[16];
    snprintf(text, sizeof(text), "%.1f N", forceNewton);

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

    int16_t x = (DISPLAY_SIZE - textWidth) / 2 - x1;
    int16_t y = (DISPLAY_SIZE - textHeight) / 2 - y1;
    tft.setCursor(x, y);
    tft.print(text);
}
