#include "TftDisplay.h"

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <strings.h> // strcasecmp

#include "HwInterface.h"

clTftDisplay TftDisplay;

namespace
{
    constexpr int16_t DISPLAY_SIZE = 240; // GC9A01A round display, 240x240

    SPIClass tftSpi(FSPI);
    Adafruit_GC9A01A tft(&tftSpi, TFT_DC, TFT_CS, TFT_RST);

    // --- "old" theme constants --------------------------------------------------------
    constexpr uint16_t OLD_THEME_LIT = GC9A01A_RED;      // lit segment
    constexpr uint16_t OLD_THEME_GHOST = 0x3000;         // dim red - always-visible "unlit" segment
    constexpr uint16_t OLD_THEME_LABEL = GC9A01A_ORANGE; // small text

    // bit0=A(top) 1=B(top-right) 2=C(bottom-right) 3=D(bottom) 4=E(bottom-left) 5=F(top-left) 6=G(middle)
    constexpr uint8_t SEVEN_SEG_DIGITS[10] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
}

void clTftDisplay::clearTextBounds(int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t bg)
{
    tft.fillRect(x1 - 1, y1 - 1, w + 2, h + 2, bg);
}

void clTftDisplay::drawAt(const char *text, int16_t x, int16_t y, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
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

void clTftDisplay::drawCenteredAt(const char *text, int16_t centerY, uint16_t bg, TextSlot *slot)
{
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

    int16_t x = (DISPLAY_SIZE - textWidth) / 2 - x1;
    int16_t y = centerY - textHeight / 2 - y1;
    drawAt(text, x, y, x + x1, y + y1, textWidth, textHeight, bg, slot);
}

void clTftDisplay::drawRightAligned(const char *text, int16_t right, int16_t y, uint16_t bg, TextSlot *slot)
{
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int16_t x = right - textWidth;
    drawAt(text, x, y, x + x1, y + y1, textWidth, textHeight, bg, slot);
}

void clTftDisplay::drawLeftAt(const char *text, int16_t x, int16_t y, uint16_t bg, TextSlot *slot)
{
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, x, y, &x1, &y1, &textWidth, &textHeight);
    drawAt(text, x, y, x1, y1, textWidth, textHeight, bg, slot);
}

void clTftDisplay::draw7SegDigit(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, int digit)
{
    uint8_t mask = (digit >= 0 && digit <= 9) ? SEVEN_SEG_DIGITS[digit] : 0x00;
    auto seg = [&](uint8_t bit, int16_t sx, int16_t sy, int16_t sw, int16_t sh)
    {
        tft.fillRect(sx, sy, sw, sh, (mask & bit) ? OLD_THEME_LIT : OLD_THEME_GHOST);
    };

    int16_t half = h / 2;
    seg(0x01, x + t, y, w - 2 * t, t);                  // A top
    seg(0x02, x + w - t, y + t, t, half - t);           // B top-right
    seg(0x04, x + w - t, y + half, t, half - t);        // C bottom-right
    seg(0x08, x + t, y + h - t, w - 2 * t, t);          // D bottom
    seg(0x10, x, y + half, t, half - t);                // E bottom-left
    seg(0x20, x, y + t, t, half - t);                   // F top-left
    seg(0x40, x + t, y + half - t / 2, w - 2 * t, t);   // G middle
}

void clTftDisplay::drawForceValueOld(float value)
{
    constexpr int16_t DIGIT_W = 22, DIGIT_H = 40, DIGIT_T = 4, GAP = 6;
    constexpr int16_t BANK_Y = 35;
    constexpr int16_t BANK_X = 55;

    bool negative = value < 0;
    float absVal = fabsf(value);
    if (absVal > 999.9f)
    {
        absVal = 999.9f;
    }
    int tenthsTotal = (int)roundf(absVal * 10.0f);
    int intPart = tenthsTotal / 10;
    int tenths = tenthsTotal % 10;
    int hundreds = intPart / 100;
    int tens = (intPart / 10) % 10;
    int ones = intPart % 10;
    bool blankHundreds = hundreds == 0;
    bool blankTens = blankHundreds && tens == 0;

    tft.fillRect(BANK_X - 16, BANK_Y + DIGIT_H / 2 - DIGIT_T / 2, 12, DIGIT_T,
                 negative ? OLD_THEME_LIT : OLD_THEME_GHOST);

    int16_t x = BANK_X;
    draw7SegDigit(x, BANK_Y, DIGIT_W, DIGIT_H, DIGIT_T, blankHundreds ? -1 : hundreds);
    x += DIGIT_W + GAP;
    draw7SegDigit(x, BANK_Y, DIGIT_W, DIGIT_H, DIGIT_T, blankTens ? -1 : tens);
    x += DIGIT_W + GAP;
    draw7SegDigit(x, BANK_Y, DIGIT_W, DIGIT_H, DIGIT_T, ones);
    x += DIGIT_W + GAP;

    constexpr int16_t DP_SIZE = 6;
    tft.fillRect(x, BANK_Y + DIGIT_H - DP_SIZE, DP_SIZE, DP_SIZE, OLD_THEME_LIT);
    x += DP_SIZE + GAP;

    draw7SegDigit(x, BANK_Y, DIGIT_W, DIGIT_H, DIGIT_T, tenths);
    x += DIGIT_W + GAP + 6;

    tft.setTextColor(OLD_THEME_LIT);
    tft.setTextSize(2);
    tft.setCursor(x, BANK_Y + DIGIT_H / 2 - 8);
    tft.print("N");
}

void clTftDisplay::drawChart(const float *values, int count, int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color, uint16_t labelColor)
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

    tft.setTextColor(labelColor);
    tft.setTextSize(1);
    char maxLabel[12];
    char minLabel[12];
    snprintf(maxLabel, sizeof(maxLabel), "%.1fN", maxV);
    snprintf(minLabel, sizeof(minLabel), "%.1fN", minV);
    drawRightAligned(maxLabel, x + w, y);
    drawRightAligned(minLabel, x + w, y + h - 8);
}

void clTftDisplay::setup(const char *theme)
{
    currentTheme = (theme && strcasecmp(theme, "old") == 0) ? TftTheme::Old : TftTheme::Default;

    tftSpi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.begin();
    tft.setRotation(2); // rotate 180 deg
    tft.fillScreen(GC9A01A_BLACK);
}

void clTftDisplay::showForce(float forceNewton, const float *history, int historyCount, const char *ipText)
{
    float roundedForce = roundf(forceNewton * 10.0f) / 10.0f;
    if (roundedForce == lastShownForce)
    {
        return;
    }
    lastShownForce = roundedForce;

    constexpr int16_t CHART_X = 30;
    constexpr int16_t CHART_Y = 95;
    constexpr int16_t CHART_W = 180;
    constexpr int16_t CHART_H = 75;
    constexpr float SAMPLE_INTERVAL_SEC = 0.2f;

    if (firstForceDraw)
    {
        tft.fillScreen(GC9A01A_BLACK);
        firstForceDraw = false;
    }

    bool isOld = currentTheme == TftTheme::Old;
    uint16_t chartColor = isOld ? OLD_THEME_LIT : GC9A01A_CYAN;
    uint16_t labelColor = isOld ? OLD_THEME_LABEL : GC9A01A_WHITE;

    if (isOld)
    {
        drawForceValueOld(forceNewton);
    }
    else
    {
        tft.setTextColor(GC9A01A_WHITE);
        tft.setTextSize(4);
        char text[16];
        snprintf(text, sizeof(text), "%.1f N", forceNewton);
        drawCenteredAt(text, 55, GC9A01A_BLACK, &forceTitleSlot);
    }

    tft.fillRect(CHART_X, CHART_Y, CHART_W + 1, CHART_H + 1, GC9A01A_BLACK);
    drawChart(history, historyCount, CHART_X, CHART_Y, CHART_W, CHART_H, chartColor, labelColor);

    float durationSec = historyCount > 1 ? (historyCount - 1) * SAMPLE_INTERVAL_SEC : 0.0f;
    tft.setTextColor(labelColor);
    tft.setTextSize(1);
    char startLabel[8];
    snprintf(startLabel, sizeof(startLabel), "-%.0fs", durationSec);
    int16_t axisLabelY = CHART_Y + CHART_H + 4;
    drawLeftAt(startLabel, CHART_X, axisLabelY, GC9A01A_BLACK, &startLabelSlot);
    drawRightAligned("0s", CHART_X + CHART_W, axisLabelY);

    tft.setTextSize(1);
    drawCenteredAt(ipText, 218, GC9A01A_BLACK, &ipTextSlot);
}

void clTftDisplay::message(const char *line1, const char *line2)
{
    tft.fillScreen(GC9A01A_BLACK);

    uint16_t color = (currentTheme == TftTheme::Old) ? OLD_THEME_LIT : GC9A01A_WHITE;
    tft.setTextColor(color);
    tft.setTextSize(3);
    drawCenteredAt(line1, 95);

    tft.setTextSize(2);
    drawCenteredAt(line2, 140);

    firstForceDraw = true;
}

void clTftDisplay::splash(const char *versionInfo)
{
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(GC9A01A_MAGENTA);
    tft.setTextSize(3);
    drawCenteredAt("Force Sensor", 100);

    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(1);
    drawCenteredAt(versionInfo, 140);
}

