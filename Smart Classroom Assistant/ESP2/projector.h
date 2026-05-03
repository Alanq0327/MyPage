#ifndef PROJECTOR_H
#define PROJECTOR_H

void projector_init(void);
void projector_power(bool on);

void projector_open();    
void projector_close();   
void projector_curtain_action(const String &cmd);

void projector_loop(void);

#endif