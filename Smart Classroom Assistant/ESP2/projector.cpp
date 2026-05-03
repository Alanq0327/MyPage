#include <Arduino.h>
#include "projector.h"
#include "config.h"

static void curtain_start_up();
static void curtain_start_down();
static void curtain_stop();

//projector LED
static bool proj_light_on = false;
//curtain
static enum {
    CURTAIN_IDLE = 0,
    CURTAIN_MOVING_UP,
    CURTAIN_MOVING_DOWN
} curtain_state = CURTAIN_IDLE;

static long curtain_start_ms = 0;
const long CURTAIN_TIME = 1000;

//initialization
void projector_init(void)
{
    pinMode(PIN_PROJECTOR_LED, OUTPUT);
    digitalWrite(PIN_PROJECTOR_LED, LOW);  

    proj_light_on = false;

    pinMode(PWMA, OUTPUT);
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);

    analogWriteRange(1023);
    analogWriteFreq(1000);

    analogWrite(PWMA, 0);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);

    Serial.println("Init OK");
}

//projector on/off
void projector_power(bool on)
{
    proj_light_on = on;

    if (on) {
        digitalWrite(PIN_PROJECTOR_LED, HIGH);   
    } else {
        digitalWrite(PIN_PROJECTOR_LED, LOW);  
    }

    Serial.printf("Projector Light = %s\n", on ? "ON" : "OFF");

    if (on)
        curtain_start_down();
    else
        curtain_start_up();
}

//curtain control
static void curtain_stop()
{
    analogWrite(PWMA, 0);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);

    curtain_state = CURTAIN_IDLE;
    Serial.println("Curtain STOP");
}

static void curtain_start_up()
{
    curtain_state = CURTAIN_MOVING_UP;
    curtain_start_ms = millis();

    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 800);

    Serial.println("Curtain UP");
}

static void curtain_start_down()
{
    curtain_state = CURTAIN_MOVING_DOWN;
    curtain_start_ms = millis();

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, 800);

    Serial.println("Curtain DOWN");
}

void projector_curtain_action(const String &cmd)
{
    if (cmd == "up") {
        curtain_start_up();
    }
    else if (cmd == "down") {
        curtain_start_down();
    }
    else {
        curtain_stop();
    }
}

void projector_loop(void)
{
    if (curtain_state == CURTAIN_IDLE)
        return;

    if (millis() - curtain_start_ms >= CURTAIN_TIME) {
        curtain_stop();
    }
}
