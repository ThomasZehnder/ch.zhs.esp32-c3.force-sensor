#include "display.h"

#include <Wire.h>
#include <U8g2lib.h>

namespace
{
constexpr int SDA_PIN = 5;
constexpr int SCL_PIN = 6;
constexpr int MAX_QUEUE_SIZE = 10;

U8G2_SSD1306_72X40_ER_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

struct DisplayItem
{
    String line1, line2, line3, line4;
    unsigned long durationMs;
    unsigned long displayStartTime;
};

DisplayItem displayQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;
unsigned long currentItemEndTime = 0;
bool isQueueMode = false;

void drawCenteredText(int y, const char *text)
{
    int width = oled.getDisplayWidth();
    int textWidth = oled.getStrWidth(text);
    int x = (width - textWidth) / 2;
    if (x < 0)
    {
        x = 0;
    }
    oled.drawStr(x, y, text);
}

void enqueueDisplay(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs)
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

bool dequeueDisplay(DisplayItem &item)
{
    if (queueCount == 0)
    {
        return false;
    }

    item = displayQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    queueCount--;

    return true;
}
}

void initDisplay()
{
    Wire.begin(SDA_PIN, SCL_PIN);
    oled.begin();
    oled.setFont(u8g2_font_5x8_tr);
}

void renderDisplay(const String &line1, const String &line2, const String &line3, const String &line4)
{
    Serial.println("[OLED] renderDisplay() called");
    Serial.print("[OLED] Line1: ");
    Serial.println(line1);

    oled.clearBuffer();
    Serial.println("[OLED] Buffer cleared");

    oled.setFont(u8g2_font_5x8_tr);
    drawCenteredText(8, line1.c_str());

    oled.setFont(u8g2_font_4x6_tr);
    drawCenteredText(18, line2.c_str());

    oled.setFont(u8g2_font_5x8_tr);
    drawCenteredText(28, line3.c_str());
    drawCenteredText(38, line4.c_str());

    Serial.println("[OLED] Text drawn, sending buffer...");
    oled.sendBuffer();
    Serial.println("[OLED] Buffer sent to OLED");
}

void renderDisplayQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs)
{
    enqueueDisplay(line1, line2, line3, line4, durationMs);
}

void displayUpdate()
{
    static unsigned long lastDebug = 0;

    // Debug output every 2 seconds
    if ((long)(millis() - lastDebug) >= 2000)
    {
        lastDebug = millis();
        Serial.print("[DISPLAY] Queue status - isQueueMode: ");
        Serial.print(isQueueMode);
        Serial.print(" queueCount: ");
        Serial.print(queueCount);
        Serial.print(" currentItemEndTime: ");
        Serial.print(currentItemEndTime);
        Serial.print(" millis: ");
        Serial.println(millis());
    }

    if (!isQueueMode || queueCount == 0)
    {
        return;
    }

    // Check if current item should expire
    long timeUntilExpire = (long)(currentItemEndTime - millis());
    if (timeUntilExpire <= 0)
    {
        DisplayItem item;
        if (dequeueDisplay(item))
        {
            Serial.print("[DISPLAY] Dequeued item: ");
            Serial.print(item.line1);
            Serial.print(" | ");
            Serial.print(item.line2);
            Serial.print(" | Duration: ");
            Serial.print(item.durationMs);
            Serial.println(" ms");

            renderDisplay(item.line1, item.line2, item.line3, item.line4);
            currentItemEndTime = millis() + item.durationMs;
        }
        else
        {
            Serial.println("[DISPLAY] Queue empty, disabling queue mode");
            isQueueMode = false;
        }
    }
}