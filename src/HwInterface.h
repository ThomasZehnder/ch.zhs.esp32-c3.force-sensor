// ESP32-C3 GPIO pin definitions
// I2C (OLED): GPIO 9 (SCL), GPIO 10 (SDA)
// LED: GPIO 8 (built-in on DevKitM-1)
/*
// GPIO pins (ESP32-C3)
#define GPIO0  0    // Button/Key
#define GPIO1  1    // UART TX / Button
#define GPIO2  2    // Button/Key
#define GPIO3  3    // USB CDC / Button
#define GPIO4  4    // SPI / GPIO
#define GPIO5  5    // SPI / GPIO
#define GPIO6  6    // Strapping pin (avoid if possible)
#define GPIO7  7    // Strapping pin (avoid if possible)
#define GPIO8  8    // Built-in LED (DevKitM-1)
#define GPIO9  9    // I2C SCL
#define GPIO10 10   // Reserved/PSRAM

*/

#define LED_PIN 8  // Built-in LED on ESP32-C3-DevKitM-1


#define KEY1_PIN       0   // Button 1 (GPIO0)
#define KEY2_PIN       2   // Button 2 (GPIO2)

#define FORCE_DOUT_PIN 4   // HX711 data output
#define FORCE_SCK_PIN  9   // HX711 clock (changed from GPIO5 to avoid OLED I2C conflict)


void hwSetup(void);
void hwLoop(void);

bool hwSecoundTick(void);
bool hwCentiSecoundTick(void);
bool hwForceSampleTick(void);
unsigned long hwGetMillis(void);

//bool keyPressed(int keyNumber);
int keyPressedCounter(int keyNumber);

void pollKeyPressed(void);
