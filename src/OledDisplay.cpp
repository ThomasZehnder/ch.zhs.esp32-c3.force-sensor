#include "OledDisplay.h"

#include <Wire.h>

namespace
{
    // OLED I2C pins (fix verlötet auf GPIO5=SDA, GPIO6=SCL)
    constexpr int SDA_PIN = 5; // I2C SDA (OLED - fix verlötet)
    constexpr int SCL_PIN = 6; // I2C SCL (OLED - fix verlötet)
    constexpr int MAX_QUEUE_SIZE = 20;
    constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 100;
}

void clOledDisplay::drawCenteredText(int y, const char *text)
{
    int width = oled.getDisplayWidth();
    int textWidth = oled.getStrWidth(text);
    int x = (width - textWidth) / 2;
    if (x < 0)
        x = 0;
    oled.drawStr(x, y, text);
}

void clOledDisplay::drawLeftText(int y, const char *text)
{
    oled.drawStr(0, y, text);
}

void clOledDisplay::drawRightText(int y, const char *text)
{
    int width = oled.getDisplayWidth();
    int textWidth = oled.getStrWidth(text);
    int x = width - textWidth;
    if (x < 0)
        x = 0;
    oled.drawStr(x, y, text);
}

void clOledDisplay::drawCenteredTextFit(int y, const char *text, const uint8_t *preferredFont, const uint8_t *fallbackFont)
{
    oled.setFont(preferredFont);
    if (oled.getStrWidth(text) > oled.getDisplayWidth())
        oled.setFont(fallbackFont);
    drawCenteredText(y, text);
}

void clOledDisplay::enqueueDisplay(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs)
{
    if (queueCount >= MAX_QUEUE_SIZE)
    {
        Serial.println("[DISPLAY] Queue is full!");
        return;
    }

    DisplayItem &item = displayQueue[queueTail];
    item.line1 = line1;
    item.line2 = line2;
    item.line3 = line3;
    item.line4 = line4;
    item.durationMs = durationMs;
    item.displayStartTime = millis();

    queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
    queueCount++;

    isQueueMode = true;
    Serial.print("[DISPLAY] Enqueued item. Queue size: ");
    Serial.println(queueCount);
}

bool clOledDisplay::dequeueDisplay(DisplayItem &item)
{
    if (queueCount == 0)
        return false;

    item = displayQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    queueCount--;

    return true;
}

void clOledDisplay::init()
{
    Wire.begin(SDA_PIN, SCL_PIN);
    oled.begin();
    oled.setFont(u8g2_font_5x8_tr);
}

void clOledDisplay::render(const String &line1, const String &line2, const String &line3, const String &line4)
{
    oled.firstPage();
    do
    {
        if (line1.startsWith("#"))
        {
            oled.setFont(u8g2_font_5x8_tr);
            drawLeftText(8, line1.c_str());
            oled.setFont(u8g2_font_logisoso18_tf);
            drawRightText(30, line2.c_str());

            oled.setFont(u8g2_font_5x8_tr);
            drawCenteredText(31, line3.c_str());
            drawCenteredText(39, line4.c_str());
        }
        else
        {
            oled.setFont(u8g2_font_5x8_tr);
            drawCenteredText(8, line1.c_str());
            drawCenteredTextFit(20, line2.c_str(), u8g2_font_8x13B_tf, u8g2_font_6x10_tf);
            oled.setFont(u8g2_font_5x8_tr);
            drawCenteredText(31, line3.c_str());
            drawCenteredText(39, line4.c_str());
        }
    } while (oled.nextPage());
}

void clOledDisplay::renderQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs)
{
    enqueueDisplay(line1, line2, line3, line4, durationMs);
}

void clOledDisplay::update()
{
    if ((long)(millis() - lastDisplayUpdateTime) < DISPLAY_UPDATE_INTERVAL_MS)
        return;

    if (!isQueueMode || queueCount == 0)
        return;

    long timeUntilExpire = (long)(currentItemEndTime - millis());
    if (timeUntilExpire <= 0)
    {
        DisplayItem item;
        if (dequeueDisplay(item))
        {
            Serial.print("[DISPLAY] Dequeued item: ");
            Serial.print(queueCount);
            Serial.print(" - ");
            Serial.print(item.line1);
            Serial.print(" | ");
            Serial.print(item.line2);
            Serial.print(" | ");
            Serial.print(item.line3);
            Serial.print(" | ");
            Serial.print(item.line4);
            Serial.print(" # Duration: ");
            Serial.print(item.durationMs);
            Serial.println(" ms");

            lastDisplayUpdateTime = millis();
            render(item.line1, item.line2, item.line3, item.line4);
            currentItemEndTime = millis() + item.durationMs;
        }
        else
        {
            Serial.println("[DISPLAY] Queue empty, disabling queue mode");
            isQueueMode = false;
        }
    }
}

bool clOledDisplay::isQueueEmpty()
{
    return queueCount == 0;
}

clOledDisplay OledDisplay;