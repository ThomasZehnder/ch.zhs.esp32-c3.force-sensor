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
    bool firstForceDraw = true;

    // remembers the last drawn bounding box of one text element across calls, so that a
    // redraw can erase exactly the *previous* box before drawing the new (possibly smaller)
    // one - a same-size self-clear would otherwise leave stray pixels when text shrinks,
    // e.g. "-12.3 N" -> "9.8 N" no longer covers the old left/right edges
    struct TextSlot
    {
        int16_t x = 0, y = 0;
        uint16_t w = 0, h = 0;
        bool valid = false;
    };

    TextSlot forceTitleSlot;
    TextSlot startLabelSlot;
    TextSlot ipTextSlot;

    void clearTextBounds(int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t bg)
    {
        tft.fillRect(x1 - 1, y1 - 1, w + 2, h + 2, bg);
    }

    // erases slot's previous box (if any) and stores the new one - pass slot=nullptr for
    // elements whose full redraw area is already guaranteed clear some other way (e.g.
    // inside a rect that just got fillRect'd, or a string that never changes length)
    void drawAt(const char *text, int16_t x, int16_t y, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                uint16_t bg, TextSlot *slot)
    {
        if (slot)
        {
            if (slot->valid)
            {
                clearTextBounds(slot->x, slot->y, slot->w, slot->h, bg);
            }
            slot->x = x1;
            slot->y = y1;
            slot->w = w;
            slot->h = h;
            slot->valid = true;
        }
        else
        {
            clearTextBounds(x1, y1, w, h, bg);
        }

        tft.setCursor(x, y);
        tft.print(text);
    }

    // horizontally centered on the full display width, vertically centered on centerY
    void drawCenteredAt(const char *text, int16_t centerY, uint16_t bg = GC9A01A_BLACK, TextSlot *slot = nullptr)
    {
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

        int16_t x = (DISPLAY_SIZE - textWidth) / 2 - x1;
        int16_t y = centerY - textHeight / 2 - y1;
        drawAt(text, x, y, x + x1, y + y1, textWidth, textHeight, bg, slot);
    }

    // right-aligned so the text ends exactly at x=right
    void drawRightAligned(const char *text, int16_t right, int16_t y, uint16_t bg = GC9A01A_BLACK, TextSlot *slot = nullptr)
    {
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int16_t x = right - textWidth;
        drawAt(text, x, y, x + x1, y + y1, textWidth, textHeight, bg, slot);
    }

    // plain left-aligned text at (x,y), same auto-clear behaviour as the helpers above
    void drawLeftAt(const char *text, int16_t x, int16_t y, uint16_t bg = GC9A01A_BLACK, TextSlot *slot = nullptr)
    {
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, x, y, &x1, &y1, &textWidth, &textHeight);
        drawAt(text, x, y, x1, y1, textWidth, textHeight, bg, slot);
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

void tftDisplayShowForce(float forceNewton, const float *history, int historyCount, const char *ipText)
{
    // round to the displayed precision (0.1N) - sensor noise on the raw float would otherwise
    // trigger a full redraw (and its black flash) on almost every 200ms tick even when nothing
    // visibly changes
    float roundedForce = roundf(forceNewton * 10.0f) / 10.0f;
    if (roundedForce == lastShownForce)
    {
        return;
    }
    lastShownForce = roundedForce;

    // chart rectangle must stay inside the round bezel - all 4 corners within radius ~115
    constexpr int16_t CHART_X = 30;
    constexpr int16_t CHART_Y = 95;
    constexpr int16_t CHART_W = 180;
    constexpr int16_t CHART_H = 75;
    // must match FORCE_SAMPLE_INTERVAL (200ms) in HwInterface.cpp - only used for the x-axis label
    constexpr float SAMPLE_INTERVAL_SEC = 0.2f;

    // one full clear on the first frame, to wipe any leftover startup-screen content (its
    // layout differs from this screen's, so the per-element clears below wouldn't reach it) -
    // every frame after that only clears its own bounding boxes, avoiding the black flash
    if (firstForceDraw)
    {
        tft.fillScreen(GC9A01A_BLACK);
        firstForceDraw = false;
    }

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(4);
    char text[16];
    snprintf(text, sizeof(text), "%.1f N", forceNewton);
    drawCenteredAt(text, 55, GC9A01A_BLACK, &forceTitleSlot);

    // +1 on both dimensions: sampleY()/px can reach exactly y+h / x+w, one row/column past
    // what a plain w x h fillRect would cover, otherwise leaving stray pixels behind there
    tft.fillRect(CHART_X, CHART_Y, CHART_W + 1, CHART_H + 1, GC9A01A_BLACK);
    drawChart(history, historyCount, CHART_X, CHART_Y, CHART_W, CHART_H, GC9A01A_CYAN);

    // x-axis: how far back in time the left edge of the chart reaches, "now" at the right edge
    float durationSec = historyCount > 1 ? (historyCount - 1) * SAMPLE_INTERVAL_SEC : 0.0f;
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(1);
    char startLabel[8];
    snprintf(startLabel, sizeof(startLabel), "-%.0fs", durationSec);
    int16_t axisLabelY = CHART_Y + CHART_H + 4;
    drawLeftAt(startLabel, CHART_X, axisLabelY, GC9A01A_BLACK, &startLabelSlot);
    drawRightAligned("0s", CHART_X + CHART_W, axisLabelY); // constant text, no shrink possible - no slot needed

    // IP address, small, at the very bottom - same info as the "#Force" line on the OLED
    tft.setTextSize(1);
    drawCenteredAt(ipText, 218, GC9A01A_BLACK, &ipTextSlot);
}

void tftDisplayTest(const char *versionInfo)
{
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(GC9A01A_MAGENTA);
    tft.setTextSize(3);
    drawCenteredAt("Force Sensor", 100);

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(1);
    drawCenteredAt(versionInfo, 140);
}
