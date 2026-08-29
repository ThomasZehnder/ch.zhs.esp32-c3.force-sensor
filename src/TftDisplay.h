#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Arduino.h>

class clTftDisplay
{
public:
    void setup(const char *theme);
    void showForce(float forceNewton, const float *history, int historyCount, const char *ipText);
    void message(const char *line1, const char *line2);
    void splash(const char *versionInfo);

private:
    enum class TftTheme
    {
        Default, // plain proportional-font text, cyan chart
        Old      // red 7-segment digit bank, red/amber chart+labels, retro meter look
    };
    TftTheme currentTheme = TftTheme::Default;

    float lastShownForce = NAN;
    bool firstForceDraw = true;

    struct TextSlot
    {
        int16_t x = 0, y = 0;
        uint16_t w = 0, h = 0;
        bool valid = false;
    };

    TextSlot forceTitleSlot;
    TextSlot startLabelSlot;
    TextSlot ipTextSlot;

    void clearTextBounds(int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t bg);
    void drawAt(const char *text, int16_t x, int16_t y, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
                uint16_t bg, TextSlot *slot);
    void drawCenteredAt(const char *text, int16_t centerY, uint16_t bg = 0x0000, TextSlot *slot = nullptr);
    void drawRightAligned(const char *text, int16_t right, int16_t y, uint16_t bg = 0x0000, TextSlot *slot = nullptr);
    void drawLeftAt(const char *text, int16_t x, int16_t y, uint16_t bg = 0x0000, TextSlot *slot = nullptr);
    void draw7SegDigit(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, int digit);
    void drawForceValueOld(float value);
    void drawChart(const float *values, int count, int16_t x, int16_t y, int16_t w, int16_t h,
                   uint16_t color, uint16_t labelColor);
};

extern clTftDisplay TftDisplay;

#endif // TFT_DISPLAY_H

