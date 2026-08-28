// ESP32-C3 GPIO pin definitions
// I2C (onboard OLED): GPIO5 (SDA), GPIO6 (SCL) - hardwired, NOT changeable!
// Onboard blue LED: GPIO8 - shared with FORCE_DOUT_PIN below, no longer used as a controllable LED

// Round GC9A01A TFT (SPI) - not yet wired up in display.cpp
#define TFT_CS   3
#define TFT_DC   2
#define TFT_RST  10
#define TFT_SCLK 4
#define TFT_MOSI 7
// Display is write-only (no MISO wired), but the ESP32-C3 core has no compiled-in
// default SPI pins and crashes if -1 is passed for MISO - GPIO20 is unused (native
// USB CDC serial is active, so the UART0 RX/TX pins are free) and just parked here.
#define TFT_MISO 20
// BLK (backlight) is hardwired to 3V3, no GPIO needed

#define KEY1_PIN       0   // Button 1 - Tare
#define KEY2_PIN       1   // Button 2 - Calibrate (40N)

#define FORCE_DOUT_PIN 8   // HX711 data output (shares the pin with the onboard LED - harmless)
#define FORCE_SCK_PIN  9   // HX711 clock

void hwSetup(void);
void hwLoop(void);

bool hwSecoundTick(void);
bool hwCentiSecoundTick(void);
bool hwForceSampleTick(void);
unsigned long hwGetMillis(void);

//bool keyPressed(int keyNumber);
int keyPressedCounter(int keyNumber);

void pollKeyPressed(void);
