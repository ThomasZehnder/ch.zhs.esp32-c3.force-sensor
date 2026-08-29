#pragma once

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>

class clOledDisplay
{
public:
    void init();
    void render(const String &line1, const String &line2, const String &line3, const String &line4);
    void renderQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs);
    void update();
    bool isQueueEmpty();
};

extern clOledDisplay OledDisplay;

#endif // OLED_DISPLAY_H
