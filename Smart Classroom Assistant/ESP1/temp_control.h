#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H

void temp_control_init(void);

void temp_set_fan(bool on);
void temp_set_heater(bool on);

void temp_set_auto(bool enable);

void temp_control_loop(void);

void temp_set_low_threshold(float low);
void temp_set_high_threshold(float high);

bool  temp_get_auto_mode();
bool  temp_get_fan_state();
bool  temp_get_heater_state();

float temp_get_low_threshold();
float temp_get_high_threshold();

#endif
