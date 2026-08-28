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

    // horizontally centered on the full display width, vertically centered on centerY
    void drawCenteredAt(const char *text, int16_t centerY)
    {
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

        int16_t x = (DISPLAY_SIZE - textWidth) / 2 - x1;
        int16_t y = centerY - textHeight / 2 - y1;
        tft.setCursor(x, y);
        tft.print(text);
    }

    // right-aligned so the text ends exactly at x=right
    void drawRightAligned(const char *text, int16_t right, int16_t y)
    {
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        tft.setCursor(right - textWidth, y);
        tft.print(text);
    }

    // draws values as a connected line, autoscaled to its own min/max - same idea as
    // drawForceChart() in data/index.html. x/y/w/h must stay inside the round bezel.
    // min/max are additionally labelled, small and right-aligned to the chart's right edge.
    void drawChart(const float *values, int count, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
    {
        if (count < 2)
        {
            return;
        }

        float minV = values[0];
        float maxV = values[0];
        for (int i = 1; i < count; i++)
        {
            if (values[i] < minV) minV = values[i];
            if (values[i] > maxV) maxV = values[i];
        }
        float range = maxV - minV;

        auto sampleY = [&](float v) -> int16_t
        {
            return y + h - (range > 0 ? (int16_t)(((v - minV) / range) * h) : h / 2);
        };

        int16_t prevX = x;
        int16_t prevY = sampleY(values[0]);
        for (int i = 1; i < count; i++)
        {
            int16_t px = x + (int16_t)(((float)i / (count - 1)) * w);
            int16_t py = sampleY(values[i]);
            tft.drawLine(prevX, prevY, px, py, color);
            prevX = px;
            prevY = py;
        }

        tft.setTextColor(GC9A01A_WHITE);
        tft.setTextSize(1);
        char maxLabel[12];
        char minLabel[12];
        snprintf(maxLabel, sizeof(maxLabel), "%.1fN", maxV);
        snprintf(minLabel, sizeof(minLabel), "%.1fN", minV);
        drawRightAligned(maxLabel, x + w, y);          // upper limit, top-right of the chart
        drawRightAligned(minLabel, x + w, y + h - 8);  // lower limit, bottom-right (8px ~ size-1 text height)
    }
}

void tftDisplaySetup()
{
    tftSpi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(2); //2 rotate 180 deg
    tft.fillScreen(GC9A01A_BLACK);
}

void tftDisplayShowForce(float forceNewton, const float *history, int historyCount)
{
    if (forceNewton == lastShownForce)
    {
        return; // unchanged since last draw, skip flicker
    }
    lastShownForce = forceNewton;

    // chart rectangle must stay inside the round bezel - all 4 corners within radius ~115
    constexpr int16_t CHART_X = 30;
    constexpr int16_t CHART_Y = 95;
    constexpr int16_t CHART_W = 180;
    constexpr int16_t CHART_H = 85;

    tft.fillScreen(GC9A01A_BLACK);

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(4);
    char text[16];
    snprintf(text, sizeof(text), "%.1f N", forceNewton);
    drawCenteredAt(text, 55);

    drawChart(history, historyCount, CHART_X, CHART_Y, CHART_W, CHART_H, GC9A01A_CYAN);
}

void tftDisplayTest()
{
    tft.fillScreen(GC9A01A_RED);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(3);
    drawCenteredAt("TFT TEST", DISPLAY_SIZE / 2);
}
