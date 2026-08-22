// ESP32-C3 GPIO pin definitions
// I2C (OLED): GPIO5 (SDA), GPIO6 (SCL) - hardwired, NOT changeable!
// LED: GPIO 8 (built-in on DevKitM-1)


#define LED_PIN 8  // Built-in LED on ESP32-C3-DevKitM-1


#define KEY1_PIN       1   // Button 1 (GPIO1) - NOT GPIO0 (boot pin)
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
