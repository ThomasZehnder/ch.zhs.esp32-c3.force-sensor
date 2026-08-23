#pragma once

#include <Arduino.h>

void initDisplay();
void renderDisplay(const String &line1, const String &line2, const String &line3, const String &line4);
void displayRenderQueued(const String &line1, const String &line2, const String &line3, const String &line4, unsigned long durationMs);
void displayUpdate();
bool isDisplayQueueEmpty();