#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>

void light_init(void);

// Manually set the brightness percentage (0~100).
void light_set_brightness(float percent);
// Automatic mode setting (enable + target lux)
void light_set_auto(bool enable, float target);
// Automatic dimming cycle (inputting the latest ambient light lux)
void light_auto_adjust(float measured_lux);
// mode switching
void light_set_mode(const String &mode);
// light turn on/off
void light_switch(bool on);
// brightness application
void light_apply(void);

#endif

