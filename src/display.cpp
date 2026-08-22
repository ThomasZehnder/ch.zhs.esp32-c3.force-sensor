#include "display.h"

#include <Wire.h>
#include <U8g2lib.h>

namespace
{
    // OLED I2C pins (fix verlötet auf GPIO5=SDA, GPIO6=SCL)
    constexpr int SDA_PIN = 5; // I2C SDA (OLED - fix verlötet)
    constexpr int SCL_PIN = 6; // I2C SCL (OLED - fix verlötet)
    constexpr int MAX_QUEUE_SIZE = 20;

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
    unsigned long lastDisplayUpdateTime = 0;
    constexpr unsigned long DISPLAY_UPDATE_INTERVAL_MS = 100; // Max 10 updates per second

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

    void drawLeftText(int y, const char *text)
    {
        oled.drawStr(0, y, text);
    }

    void drawRightText(int y, const char *text)
    {
        int width = oled.getDisplayWidth();
        int textWidth = oled.getStrWidth(text);
        int x = width - textWidth;
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
    //Serial.println("[OLED] renderDisplay() called");

    // Use firstPage/nextPage instead of sendBuffer (non-blocking approach)
    oled.firstPage();
    do
    {
        if (line1.startsWith ("#"))
        {
            oled.setFont(u8g2_font_5x8_tr);
            drawLeftText(8, line1.c_str());
            oled.setFont(u8g2_font_logisoso16_tf );
            drawRightText(28, line2.c_str());
        }
        else
        {
            oled.setFont(u8g2_font_5x8_tr);
            drawCenteredText(8, line1.c_str());

            oled.setFont(u8g2_font_4x6_tr);
            drawCenteredText(18, line2.c_str());

            oled.setFont(u8g2_font_5x8_tr);
            drawCenteredText(28, line3.c_str());
            drawCenteredText(38, line4.c_str());
        }
    } while (oled.nextPage());

    //Serial.println("[OLED] Display updated");
}

void renderDisplayQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs)
{
    enqueueDisplay(line1, line2, line3, line4, durationMs);
}

void displayUpdate()
{
    static unsigned long lastDebug = 0;

    // Throttle OLED updates to avoid blocking the loop
    if ((long)(millis() - lastDisplayUpdateTime) < DISPLAY_UPDATE_INTERVAL_MS)
    {
        return;
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
            Serial.print(" | ");
            Serial.print(item.line3);
            Serial.print(" | ");
            Serial.print(item.line4);
            Serial.print(" # Duration: ");
            Serial.print(item.durationMs);
            Serial.println(" ms");

            lastDisplayUpdateTime = millis();
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