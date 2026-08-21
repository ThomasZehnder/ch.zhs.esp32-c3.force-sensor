#include <Arduino.h>
#include <HwInterface.h>


#include "Global.h"

// interval at which to blink (milliseconds)
const long SECOUND_INTERVAL = 1000;
unsigned long preSecoundMillis = 0;
bool secoundTick = false;

// 50ms tick (milliseconds)
const long CENTI_SECOUND_INTERVAL = 50;
unsigned long preCentiSecoundMillis = 0;
bool centiSecoundTick = false;

// 200ms tick used to sample the force sensor (milliseconds)
const long FORCE_SAMPLE_INTERVAL = 200;
unsigned long preForceSampleMillis = 0;
bool forceSampleTick = false;

unsigned long currentMillis = 0;


bool hwCentiSecoundTick(void)
{
    return centiSecoundTick;
}
bool hwSecoundTick(void)
{
    return secoundTick;
}
bool hwForceSampleTick(void)
{
    return forceSampleTick;
}

unsigned long hwGetMillis(void)
{
    return currentMillis;
}

void hwSetup(void)
{

    Assembly.keys[0].pin = KEY1_PIN;
    Assembly.keys[1].pin = KEY2_PIN;

    int i = 0;
    for (i = 0; i < NBRKEYS; i++)
    {
        pinMode(Assembly.keys[i].pin, INPUT_PULLUP);
    }
}

void hwLoop(void)
{
    // Stromfresser
    // digitalWrite(D8, (currentMillis % 30000) > 25000);

    currentMillis = millis();
    // secound tick
    if ((long)(currentMillis - preSecoundMillis) > 0)
    {
        preSecoundMillis = currentMillis + SECOUND_INTERVAL;
        secoundTick = true;

    }
    else
    {
        secoundTick = false;
    }
    // 50ms tick
    if ((long)(currentMillis - preCentiSecoundMillis) > 0)
    {
        preCentiSecoundMillis = currentMillis + CENTI_SECOUND_INTERVAL;
        centiSecoundTick = true;
    }
    else
    {
        centiSecoundTick = false;
    }
    // 200ms force sample tick
    if ((long)(currentMillis - preForceSampleMillis) > 0)
    {
        preForceSampleMillis = currentMillis + FORCE_SAMPLE_INTERVAL;
        forceSampleTick = true;
    }
    else
    {
        forceSampleTick = false;
    }
}

// getter
bool keyEdge(int keyNumber)
{
    return Assembly.keys[keyNumber].edge;
}
int keyPressedCounter(int keyNumber)
{
    return Assembly.keys[keyNumber].pressedCounter;
}

void pollKeyPressed(void)
{
    int i = 0;
    for (i = 0; i < NBRKEYS; i++)
    {
        bool pressed = digitalRead(Assembly.keys[i].pin);
        if ((pressed == false) && (pressed != Assembly.keys[i].pressed))
        {
            Assembly.keys[i].edge = true;
            Assembly.keys[i].pressedCounter++;
            Serial.print("[KEY] Button ");
            Serial.print(i);
            Serial.println(" pressed!");
        }
        else
        {
            Assembly.keys[i].edge = false;
        }
        Assembly.keys[i].pressed = pressed;
    }
}

