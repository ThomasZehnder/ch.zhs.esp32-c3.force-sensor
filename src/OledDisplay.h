#pragma once

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>

struct DisplayItem
{
    String line1, line2, line3, line4;
    unsigned long durationMs;
    unsigned long displayStartTime;
};

class clOledDisplay
{
private:
    U8G2_SSD1306_72X40_ER_F_HW_I2C oled{U8G2_R0, U8X8_PIN_NONE};
    DisplayItem displayQueue[20];
    int queueHead = 0;
    int queueTail = 0;
    int queueCount = 0;
    unsigned long currentItemEndTime = 0;
    bool isQueueMode = false;
    unsigned long lastDisplayUpdateTime = 0;

    void drawCenteredText(int y, const char *text);
    void drawLeftText(int y, const char *text);
    void drawRightText(int y, const char *text);
    void drawCenteredTextFit(int y, const char *text, const uint8_t *preferredFont, const uint8_t *fallbackFont);
    void enqueueDisplay(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs);
    bool dequeueDisplay(DisplayItem &item);

public:
    void init();
    void render(const String &line1, const String &line2, const String &line3, const String &line4);
    void renderQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs);
    void update();
    bool isQueueEmpty();
};

extern clOledDisplay OledDisplay;

#endif // OLED_DISPLAY_H
