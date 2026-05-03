import os
import pygame
import pigame
import time
from pygame.locals import *
import RPi.GPIO as GPIO
import subprocess
from mqtt import mqtt_rpi

# Screen size
SCREEN_W, SCREEN_H = 320, 240 

# Colors
BG_COLOR   = (20, 25, 30)
BUTTON_COLOR  = (50, 60, 70)
QUIT_COLOR = (150, 40, 40)
TEXT_COLOR = (255, 255, 255)

#  display Initalization
def init_display():
    os.putenv('SDL_VIDEODRIVER','fbcon')
    os.putenv('SDL_FBDEV','/dev/fb0')
    os.putenv('SDL_MOUSEDRV','dummy')
    os.putenv('SDL_MOUSEDEV','/dev/null')
    os.putenv('DISPLAY','')
    pygame.init()

#bailout button GPIO
BAILOUT_PIN = 27 

def init_gpio():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(BAILOUT_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

def check_bailout():
    return GPIO.input(BAILOUT_PIN) == GPIO.LOW
    
def center(font_btn, screen, rect, txt):
    label = font_btn.render(txt, True, TEXT_COLOR)
    screen.blit(label, (
        rect.centerx - label.get_width()//2,
        rect.centery - label.get_height()//2
    ))

ui_state = "home"
# Voice state on home page
voice_on = False  

#  HOME PAGE
def draw_home(screen, font_title, font_btn):
    global voice_on

    screen.fill(BG_COLOR)

    title = font_title.render("Classroom Assistant", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 15))

    BUTTON_W = 110
    BUTTON_H = 35

    temp_rect  = pygame.Rect(25, 80, BUTTON_W, BUTTON_H)
    light_rect = pygame.Rect(185, 80, BUTTON_W, BUTTON_H)
    proj_rect  = pygame.Rect(25, 130, BUTTON_W, BUTTON_H)
    voice_rect = pygame.Rect(185, 130, BUTTON_W, BUTTON_H)
    quit_rect  = pygame.Rect(105, 180, BUTTON_W, BUTTON_H)

    pygame.draw.rect(screen, BUTTON_COLOR, temp_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, light_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, proj_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, voice_rect)
    pygame.draw.rect(screen, QUIT_COLOR, quit_rect)

    center(font_btn, screen, temp_rect,  "Temperature")
    center(font_btn, screen, light_rect, "Lighting")
    center(font_btn, screen, proj_rect,  "Projector")

    voice_label = "Voice: ON" if voice_on else "Voice: OFF"
    center(font_btn, screen, voice_rect, voice_label)
    center(font_btn, screen, quit_rect,  "Quit")

    return temp_rect, light_rect, proj_rect, voice_rect, quit_rect


def handle_home(events, temp_rect, light_rect, proj_rect, voice_rect, quit_rect):
    global ui_state
    global voice_on   

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x, y = pygame.mouse.get_pos()

            if temp_rect.collidepoint(x,y):
                ui_state = "temp_main"
            elif light_rect.collidepoint(x,y):
                ui_state = "light_main"
            elif proj_rect.collidepoint(x,y):
                ui_state = "proj_main"
            elif voice_rect.collidepoint(x,y):
                voice_on = not voice_on
                print(f"Voice {'ON' if voice_on else 'OFF'}")
                if voice_on:
                    my_cmd = 'echo "start\n" > voice_fifo'
                    subprocess.check_output(my_cmd, shell=True)
                else:
                    my_cmd = 'echo "stop\n" > voice_fifo'
                    subprocess.check_output(my_cmd, shell=True)
            elif quit_rect.collidepoint(x,y):
                pygame.quit()
                GPIO.cleanup()
                exit(0)
                
                
#  Temperature Main Screen
def draw_temp_main(screen, font_title, font_btn, current_temp):

    screen.fill(BG_COLOR)

    title = font_title.render("Temp Control", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    temp_label = font_btn.render(f"Temp: {current_temp}", True, TEXT_COLOR)
    screen.blit(temp_label, (SCREEN_W//2 - temp_label.get_width()//2, 55))

    BUTTON_W = 100
    BUTTON_H = 35

    manual_rect = pygame.Rect(40, 110, BUTTON_W, BUTTON_H)
    auto_rect   = pygame.Rect(180, 110, BUTTON_W, BUTTON_H)
    back_rect   = pygame.Rect(245, 200, 65, 30)

    pygame.draw.rect(screen, BUTTON_COLOR, manual_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, auto_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, back_rect)

    center(font_btn, screen, manual_rect, "Manual")
    center(font_btn, screen, auto_rect,   "Auto")
    center(font_btn, screen, back_rect,   "Back")

    return manual_rect, auto_rect, back_rect


def handle_temp_main(events, manual_rect, auto_rect, back_rect, mqtt_rpi):
    global ui_state, fan_on, heater_on   

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x,y = pygame.mouse.get_pos()

            if manual_rect.collidepoint(x,y):
                ui_state = "temp_manual"

                fan_on = False      
                heater_on = False
                mqtt_rpi.temp_mode("manual")  

            elif auto_rect.collidepoint(x,y):
                ui_state = "temp_auto"

                fan_on = False      
                heater_on = False
                mqtt_rpi.temp_mode("auto")    

            elif back_rect.collidepoint(x,y):
                ui_state = "home"

#  temp manual page
def draw_temp_manual(screen, font_title, font_btn,
                     current_temp, fan_on, heater_on):

    screen.fill(BG_COLOR)

    # title
    title = font_title.render("Manual Mode", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    # Temperature display
    temp_label = font_btn.render(f"Temp: {current_temp}", True, TEXT_COLOR)
    screen.blit(temp_label, (SCREEN_W//2 - temp_label.get_width()//2, 55))

    # Buttons
    BUTTON_W = 160
    BUTTON_H = 40

    fan_rect    = pygame.Rect(80, 100, BUTTON_W, BUTTON_H)
    heater_rect = pygame.Rect(80, 150, BUTTON_W, BUTTON_H)
    back_rect   = pygame.Rect(245, 200, 65, 30)

    # Draw buttons
    pygame.draw.rect(screen, BUTTON_COLOR, fan_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, heater_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, back_rect)

    # Dynamic ON/OFF labels
    fan_text    = "Fan: ON"    if fan_on    else "Fan: OFF"
    heater_text = "Heater: ON" if heater_on else "Heater: OFF"

    center(font_btn, screen, fan_rect,    fan_text)
    center(font_btn, screen, heater_rect, heater_text)
    center(font_btn, screen, back_rect,   "Back")

    return fan_rect, heater_rect, back_rect


def handle_temp_manual(events, fan_rect, heater_rect, back_rect,
                       fan_on, heater_on, mqtt_rpi):

    global ui_state

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x, y = pygame.mouse.get_pos()

            # Fan toggle
            if fan_rect.collidepoint(x, y):
                fan_on = not fan_on

                if fan_on:
                    mqtt_rpi.fan_control(True)   
                    heater_on = False           
                    mqtt_rpi.heater_control(False)
                else:
                    mqtt_rpi.fan_control(False)

            # Heater toggle
            elif heater_rect.collidepoint(x, y):
                heater_on = not heater_on

                if heater_on:
                    mqtt_rpi.heater_control(True) 
                    fan_on = False                
                    mqtt_rpi.fan_control(False)
                else:
                    mqtt_rpi.heater_control(False)

            # Back
            elif back_rect.collidepoint(x, y):
                ui_state = "temp_main"

    return fan_on, heater_on


#  temp auto page
def draw_temp_auto(screen, font_title, font_btn,
                   current_temp, T_low, T_high, system_state):

    screen.fill(BG_COLOR)

    # Title
    title = font_title.render("Auto Mode", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    # Current Temperature
    temp_label = font_btn.render(f"Temp: {current_temp}", True, TEXT_COLOR)
    screen.blit(temp_label, (SCREEN_W//2 - temp_label.get_width()//2, 45))

    # System State
    state_label = font_btn.render(f"System: {system_state}", True, TEXT_COLOR)
    screen.blit(state_label, (SCREEN_W//2 - state_label.get_width()//2, 70))

    # Buttons & Layout
    BUTTON_W = 40  
    BUTTON_H = 30

    # T_low
    low_label = font_btn.render(f"T_low: {T_low} C", True, TEXT_COLOR)
    screen.blit(low_label, (40, 115))

    low_minus_rect = pygame.Rect(150, 110, BUTTON_W, BUTTON_H)
    low_plus_rect  = pygame.Rect(200, 110, BUTTON_W, BUTTON_H)

    pygame.draw.rect(screen, BUTTON_COLOR, low_minus_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, low_plus_rect)

    # T_high
    high_label = font_btn.render(f"T_high: {T_high} C", True, TEXT_COLOR)
    screen.blit(high_label, (40, 155))

    high_minus_rect = pygame.Rect(150, 150, BUTTON_W, BUTTON_H)
    high_plus_rect  = pygame.Rect(200, 150, BUTTON_W, BUTTON_H)

    pygame.draw.rect(screen, BUTTON_COLOR, high_minus_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, high_plus_rect)

    # Button text
    minus_label = font_btn.render("-", True, TEXT_COLOR)
    plus_label  = font_btn.render("+", True, TEXT_COLOR)

    screen.blit(minus_label, (low_minus_rect.centerx - minus_label.get_width()//2,
                              low_minus_rect.centery - minus_label.get_height()//2))
    screen.blit(plus_label,  (low_plus_rect.centerx - plus_label.get_width()//2,
                              low_plus_rect.centery - plus_label.get_height()//2))

    screen.blit(minus_label, (high_minus_rect.centerx - minus_label.get_width()//2,
                              high_minus_rect.centery - minus_label.get_height()//2))
    screen.blit(plus_label,  (high_plus_rect.centerx - plus_label.get_width()//2,
                              high_plus_rect.centery - plus_label.get_height()//2))

    # Back Button
    back_rect = pygame.Rect(245, 200, 65, 30)
    pygame.draw.rect(screen, BUTTON_COLOR, back_rect)

    back_label = font_btn.render("Back", True, TEXT_COLOR)
    screen.blit(back_label, (back_rect.centerx - back_label.get_width()//2,
                             back_rect.centery - back_label.get_height()//2))

    return low_minus_rect, low_plus_rect, high_minus_rect, high_plus_rect, back_rect


def handle_temp_auto(events,
                     low_minus_rect, low_plus_rect,
                     high_minus_rect, high_plus_rect,
                     back_rect,
                     temp_values,
                     mqtt_rpi):

    global ui_state
    
    changed = False

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x, y = pygame.mouse.get_pos()

            # Adjust T_low
            if low_minus_rect.collidepoint(x,y):
                temp_values["T_low"] -= 1
                if temp_values["T_low"] < 0:
                    temp_values["T_low"] = 0
                changed = True
            elif low_plus_rect.collidepoint(x,y):
                if temp_values["T_low"] < temp_values["T_high"]:
                    temp_values["T_low"] += 1
                changed = True
            elif high_minus_rect.collidepoint(x,y):
                if temp_values["T_high"] > temp_values["T_low"]:
                    temp_values["T_high"] -= 1
                changed = True
            elif high_plus_rect.collidepoint(x,y):
                temp_values["T_high"] += 1
                if temp_values["T_high"] > 99:
                    temp_values["T_high"] = 99
                changed = True
            # Back button
            elif back_rect.collidepoint(x,y):
                ui_state = "temp_main"
    if changed:
        mqtt_rpi.set_temp_thresholds(
            temp_values["T_low"],
            temp_values["T_high"]
        )

#  light main page
def draw_light_main(screen, font_title, font_btn, env_light):

    screen.fill(BG_COLOR)

    # Title
    title = font_title.render("Light Control", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    # Env Light
    env_s = "-- lx" if env_light is None else f"{env_light} lx"
    env_label = font_btn.render(f"Env Light: {env_s}", True, TEXT_COLOR)
    screen.blit(env_label, (SCREEN_W//2 - env_label.get_width()//2, 45))

    # Buttons
    BUTTON_W = 100
    BUTTON_H = 35

    manual_rect = pygame.Rect(40, 110, BUTTON_W, BUTTON_H)
    auto_rect   = pygame.Rect(180, 110, BUTTON_W, BUTTON_H)
    back_rect   = pygame.Rect(245, 200, 65, 30)

    pygame.draw.rect(screen, BUTTON_COLOR, manual_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, auto_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, back_rect)

    center(font_btn, screen, manual_rect, "Manual")
    center(font_btn, screen, auto_rect,   "Auto")
    center(font_btn, screen, back_rect,   "Back")

    return manual_rect, auto_rect, back_rect


def handle_light_main(events, manual_rect, auto_rect, back_rect):
    global ui_state

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x,y = pygame.mouse.get_pos()

            if manual_rect.collidepoint(x,y):
                ui_state = "light_manual"

            elif auto_rect.collidepoint(x,y):
                ui_state = "light_auto"

            elif back_rect.collidepoint(x,y):
                ui_state = "home"


#  light manual page  
def draw_light_manual(screen, font_title, font_btn,
                      brightness, mode, light_on, env_light):

    screen.fill(BG_COLOR)

    # Title
    title = font_title.render("Manual Mode", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    
    env_s = "-- lx" if env_light is None else f"{env_light} lx"
    env_label = font_btn.render(f"Env: {env_s}", True, TEXT_COLOR)

    btext = font_btn.render(f"Brightness: {brightness}%", True, TEXT_COLOR)

    screen.blit(env_label, (20, 45))
    screen.blit(btext, (SCREEN_W - btext.get_width() - 20, 45))

  
    mtext = font_btn.render(f"Mode: {mode}", True, TEXT_COLOR)
    screen.blit(mtext, (SCREEN_W//2 - mtext.get_width()//2, 80))


    BUTTON_W = 120
    BUTTON_H = 40

    bright_plus_rect  = pygame.Rect(40, 100, BUTTON_W, BUTTON_H)
    bright_minus_rect = pygame.Rect(180, 100, BUTTON_W, BUTTON_H)

    mode_minus_rect = pygame.Rect(40, 150, BUTTON_W, BUTTON_H)
    mode_plus_rect  = pygame.Rect(180,150, BUTTON_W, BUTTON_H)

    light_rect = pygame.Rect(80, 200, 160, 35)
    back_rect  = pygame.Rect(245, 200, 65, 30)

    for rect in [bright_plus_rect, bright_minus_rect,
                 mode_minus_rect, mode_plus_rect,
                 light_rect, back_rect]:
        pygame.draw.rect(screen, BUTTON_COLOR, rect)



    center(font_btn, screen, bright_plus_rect,  "Bright +")
    center(font_btn, screen, bright_minus_rect, "Bright -")
    center(font_btn, screen, mode_minus_rect, "< Mode")
    center(font_btn, screen, mode_plus_rect,  "Mode >")

    ltxt = "Light: ON" if light_on else "Light: OFF"
    center(font_btn, screen, light_rect, ltxt)

    center(font_btn, screen, back_rect, "Back")

    return (
        bright_plus_rect, bright_minus_rect,
        mode_minus_rect, mode_plus_rect,
        light_rect, back_rect
    )


def handle_light_manual(events,
                        bright_plus_rect, bright_minus_rect,
                        mode_minus_rect, mode_plus_rect,
                        light_rect, back_rect,
                        brightness, mode, light_on,
                        mqtt_rpi): 


    global ui_state

    modes = ["Eye Comfort", "Normal"]
    m_index = modes.index(mode)

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x,y = pygame.mouse.get_pos()

            # Brightness
            if bright_plus_rect.collidepoint(x,y):
                brightness = min(100, brightness + 10)
                mqtt_rpi.light_pwm(brightness)

            elif bright_minus_rect.collidepoint(x,y):
                brightness = max(0, brightness - 10)
                mqtt_rpi.light_pwm(brightness)

            # Mode switching
            elif mode_minus_rect.collidepoint(x,y):
                m_index = (m_index - 1) % len(modes)
                mode = modes[m_index]
                
                mqtt_rpi.light_mode("normal" if mode=="Normal" else "eye")

            elif mode_plus_rect.collidepoint(x,y):
                m_index = (m_index + 1) % len(modes)
                mode = modes[m_index]
                
                mqtt_rpi.light_mode("normal" if mode=="Normal" else "eye")

            # Light ON/OFF
            elif light_rect.collidepoint(x,y):
                light_on = not light_on
                mqtt_rpi.light_switch(light_on)

            # Back
            elif back_rect.collidepoint(x,y):
                ui_state = "light_main"

    return brightness, mode, light_on


#  light auto page
def draw_light_auto(screen, font_title, font_btn, auto_values):

    screen.fill(BG_COLOR)

    # Title
    title = font_title.render("Auto Light Mode", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    # Current Environment Lux
    env = auto_values["env_light"]
    env_s = "-- lx" if env is None else f"{env} lx"
    env_label = font_btn.render(f"Env Light: {env_s}", True, TEXT_COLOR)
    screen.blit(env_label, (SCREEN_W//2 - env_label.get_width()//2, 50))

    # Target Lux
    tgt = auto_values["target_lux"]
    tgt_label = font_btn.render(f"Target Lux: {tgt}", True, TEXT_COLOR)
    screen.blit(tgt_label, (SCREEN_W//2 - tgt_label.get_width()//2, 90))

    # Buttons (+ / -)
    BUTTON_W = 50
    BUTTON_H = 35

    minus_rect = pygame.Rect(80, 130, BUTTON_W, BUTTON_H)
    plus_rect  = pygame.Rect(190, 130, BUTTON_W, BUTTON_H)

    pygame.draw.rect(screen, BUTTON_COLOR, minus_rect)
    pygame.draw.rect(screen, BUTTON_COLOR, plus_rect)

    center(font_btn, screen, minus_rect, "-")
    center(font_btn, screen, plus_rect, "+")

    # Back button
    back_rect = pygame.Rect(245, 200, 65, 30)
    pygame.draw.rect(screen, BUTTON_COLOR, back_rect)
    center(font_btn, screen, back_rect, "Back")

    return minus_rect, plus_rect, back_rect


def handle_light_auto(events,
                      minus_rect, plus_rect, back_rect,
                      auto_values, mqtt_rpi):

    global ui_state
    changed = False

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x, y = pygame.mouse.get_pos()

            # decrease target lux
            if minus_rect.collidepoint(x, y):
                auto_values["target_lux"] = max(0, auto_values["target_lux"] - 20)
                changed = True

            # increase target lux
            elif plus_rect.collidepoint(x, y):
                auto_values["target_lux"] += 20
                changed = True

            # go back
            elif back_rect.collidepoint(x, y):
                ui_state = "light_main"

    if changed:
        target = auto_values["target_lux"]
        mqtt_rpi.light_auto(f"auto_on:{target}")


#  projector main page
def draw_proj_main(screen, font_title, font_btn, proj_mode):

    screen.fill(BG_COLOR)

    # Title
    title = font_title.render("Projector Control", True, TEXT_COLOR)
    screen.blit(title, (SCREEN_W//2 - title.get_width()//2, 10))

    # Current Mode
    mode_text = font_btn.render(f"Current: {proj_mode}", True, TEXT_COLOR)
    screen.blit(mode_text, (SCREEN_W//2 - mode_text.get_width()//2, 45))

    # Buttons
    BUTTON_W = 160
    BUTTON_H = 40

    proj_rect = pygame.Rect(80, 80, BUTTON_W, BUTTON_H)
    off_rect  = pygame.Rect(80, 130, BUTTON_W, BUTTON_H)
    movie_rect = pygame.Rect(80, 180, BUTTON_W, BUTTON_H)
    back_rect  = pygame.Rect(245, 200, 65, 30)

    for rect in [proj_rect, off_rect, movie_rect, back_rect]:
        pygame.draw.rect(screen, BUTTON_COLOR, rect)


    center(font_btn, screen, proj_rect, "Projection Mode")
    center(font_btn, screen, off_rect, "Turn Off")
    center(font_btn, screen, movie_rect, "Movie Mode")
    center(font_btn, screen, back_rect, "Back")

    return proj_rect, off_rect, movie_rect, back_rect


def handle_proj_main(events,
                     proj_rect, off_rect, movie_rect, back_rect,
                     proj_mode,
                     light_on,
                     mqtt_rpi):

    global ui_state
    global auto_values

    for e in events:
        if e.type == MOUSEBUTTONDOWN:
            x, y = pygame.mouse.get_pos()

            # Projection Mode
            if proj_rect.collidepoint(x, y):

                if proj_mode == "movie":
                    return proj_mode, light_on

                proj_mode = "projection"
                mqtt_rpi.projector_motor("down")
                mqtt_rpi.proj_power(True)
                print(1)


            # Movie Mode
            elif movie_rect.collidepoint(x, y):

                if proj_mode == "projection":
                    return proj_mode, light_on

                proj_mode = "movie"

                mqtt_rpi.projector_motor("down")
                mqtt_rpi.proj_power(True)             
                light_on = False
                mqtt_rpi.light_switch(False)

                print(0)


            # Turn OFF Projector
            elif off_rect.collidepoint(x, y):

                previous = proj_mode
                proj_mode = "off"
                mqtt_rpi.proj_power(False)

                
                if previous == "movie":

                    light_on = True
                    mqtt_rpi.light_switch(True)

            elif back_rect.collidepoint(x, y):
                ui_state = "home"

    return proj_mode, light_on



def placeholder(screen, font_btn, text):
    screen.fill(BG_COLOR)
    label = font_btn.render(text, True, TEXT_COLOR)
    screen.blit(label, (
        SCREEN_W//2 - label.get_width()//2,
        SCREEN_H//2 - label.get_height()//2
    ))







#main

temp_values = {"T_low": 20, "T_high": 26}
auto_values = {
    "env_light": None,
    "target_lux": 300   
}

brightness = 80
mode = "Eye Comfort"
light_on = False

# Temp manual values
fan_on = False
heater_on = False
proj_mode = "off"  

init_display()
init_gpio()

pitft = pigame.PiTft()
screen = pygame.display.set_mode((SCREEN_W, SCREEN_H))

font_title = pygame.font.Font(None, 30)
font_btn   = pygame.font.Font(None, 20)

clock = pygame.time.Clock()


while True:
    pitft.update()

    # bailout button
    if check_bailout():
        pygame.quit()
        GPIO.cleanup()
        exit(0)

    events = pygame.event.get()
    auto_values["env_light"] = mqtt_rpi.current_lux
    for e in events:
        if e.type == KEYDOWN and e.key == K_ESCAPE:
            pygame.quit()
            GPIO.cleanup()
            exit(0)

    # HOME PAGE
    if ui_state == "home":
        temp_rect, light_rect, proj_rect, voice_rect, quit_rect = draw_home(
            screen, font_title, font_btn
        )
        handle_home(events, temp_rect, light_rect, proj_rect, voice_rect, quit_rect)

    # TEMP MAIN PAGE
    elif ui_state == "temp_main":
        current_temp = mqtt_rpi.current_temp
        manual_rect, auto_rect, back_rect = draw_temp_main(
            screen, font_title, font_btn, current_temp
        )
        handle_temp_main(events, manual_rect, auto_rect, back_rect, mqtt_rpi)  
    # TEMP MANUAL MODE
    elif ui_state == "temp_manual":
        current_temp = mqtt_rpi.current_temp

        fan_rect, heater_rect, back_rect = draw_temp_manual(
            screen, font_title, font_btn,
            current_temp, fan_on, heater_on
        )

        fan_on, heater_on = handle_temp_manual(
            events, fan_rect, heater_rect, back_rect, fan_on, heater_on, mqtt_rpi
        )

    # TEMP AUTO MODE
    elif ui_state == "temp_auto":

        current_temp = mqtt_rpi.current_temp
        if current_temp > temp_values["T_high"]:
            system_state = "fan on"        
        elif current_temp < temp_values["T_low"]:
            system_state = "heater on"     
        else:
            system_state = "all close"     

        low_minus, low_plus, high_minus, high_plus, back_rect = draw_temp_auto(
            screen, font_title, font_btn,
            current_temp,
            temp_values["T_low"],
            temp_values["T_high"],
            system_state
        )

        handle_temp_auto(
            events,
            low_minus, low_plus,
            high_minus, high_plus,
            back_rect,
            temp_values,
            mqtt_rpi    
        )

    # LIGHT MAIN
    elif ui_state == "light_main":

        manual_rect, auto_rect, back_rect = draw_light_main(
            screen, font_title, font_btn,
            auto_values["env_light"]
        )
        handle_light_main(events, manual_rect, auto_rect, back_rect)

    # LIGHT MANUAL MODE
    elif ui_state == "light_manual":

        (bright_plus_rect, bright_minus_rect,
         mode_minus_rect, mode_plus_rect,
         light_rect, back_rect) = draw_light_manual(
            screen, font_title, font_btn,
            brightness, mode, light_on,
            auto_values["env_light"]
        )

        brightness, mode, light_on = handle_light_manual(
            events,
            bright_plus_rect, bright_minus_rect,
            mode_minus_rect, mode_plus_rect,
            light_rect, back_rect,
            brightness, mode, light_on,
            mqtt_rpi
        )

    # LIGHT AUTO MODE
    elif ui_state == "light_auto":

        minus_rect, plus_rect, back_rect = draw_light_auto(
            screen, font_title, font_btn, auto_values
        )

        handle_light_auto(
            events,
            minus_rect, plus_rect, back_rect,
            auto_values,
            mqtt_rpi
        )

    # PROJECTOR MAIN
    elif ui_state == "proj_main":

        proj_rect, off_rect, movie_rect, back_rect = draw_proj_main(
            screen, font_title, font_btn,
            proj_mode
        )

        proj_mode, light_on = handle_proj_main(
            events,
            proj_rect, off_rect, movie_rect, back_rect,
            proj_mode,
            light_on,
            mqtt_rpi
        )


    pygame.display.flip()
    time.sleep(0.01)

del(pitft)
GPIO.cleanup()
