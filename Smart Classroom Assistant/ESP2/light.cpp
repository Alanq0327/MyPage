#include <Arduino.h>
#include <NeoPixelBus.h>
#include "light.h"
#include "config.h"

const uint16_t PixelCount = LED_COUNT;
const uint8_t  PixelPin   = PIN_LED_STRIP;

NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(PixelCount, PixelPin);

static bool  auto_mode          = true;
static bool  light_enabled      = true;
static float brightness_percent = 50;
static float target_lux         = 350;

static float saved_brightness   = 50;

typedef enum {
    COLOR_NORMAL = 0,
    COLOR_EYE    = 1
} ColorMode;

static ColorMode color_mode = COLOR_NORMAL;


void light_apply()
{
    if (!light_enabled || brightness_percent <= 0)
    {
        for (int i = 0; i < PixelCount; i++)
            strip.SetPixelColor(i, RgbColor(0, 0, 0));

        strip.Show();
        return;
    }

    float scale = brightness_percent / 100.0;

    RgbColor baseColor =
        (color_mode == COLOR_EYE)
        ? RgbColor(255, 180, 80)
        : RgbColor(255, 255, 255);

    RgbColor finalColor(
        baseColor.R * scale,
        baseColor.G * scale,
        baseColor.B * scale
    );

    for (int i = 0; i < PixelCount; i++)
        strip.SetPixelColor(i, finalColor);

    strip.Show();
}


void light_init(void)
{
    strip.Begin();
    strip.Show();

    auto_mode          = true;
    light_enabled      = true;
    brightness_percent = 50;
    saved_brightness   = 50;
    target_lux         = 350;
    color_mode         = COLOR_NORMAL;

    light_apply();

    Serial.println("Init OK");
}


//manual brightness
void light_set_brightness(float percent)
{
    auto_mode = false;

    percent = constrain(percent, 0.0f, 100.0f);
    brightness_percent = percent;
    if (percent > 0)
        saved_brightness = percent;

    light_enabled = (percent > 0);

    light_apply();
}

//mode switiching
void light_set_mode(const String& mode)
{
    if (mode == "eye")
        color_mode = COLOR_EYE;
    else
        color_mode = COLOR_NORMAL;

    light_apply();
}

//turn on/off light
void light_switch(bool on)
{
    if (on)
    {
        light_enabled = true;

        if (brightness_percent <= 0)
        {
            if (saved_brightness <= 0)
                saved_brightness = 50;  

            brightness_percent = saved_brightness;
        }
    }
    else
    {
        light_enabled = false;

        if (brightness_percent > 0)
            saved_brightness = brightness_percent;
        brightness_percent = 0;
    }

    auto_mode = false;
    light_apply();
}

//auto mode
void light_set_auto(bool enable, float target)
{
    auto_mode = enable;

    if (enable && target > 0)
        target_lux = target;
}

void light_auto_adjust(float measured_lux)
{
    if (!auto_mode || !light_enabled){
        Serial.println(2345678);
        return;
    }
    float error = target_lux - measured_lux;
    float Kp = 0.05;

    brightness_percent += Kp * error;
    brightness_percent = constrain(brightness_percent, 0.0f, 100.0f);

    if (brightness_percent > 0)
        saved_brightness = brightness_percent;

    light_apply();
}