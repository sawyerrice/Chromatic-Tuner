/*****************************************************************************
* lab2a.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#ifndef lab2a_h
#define lab2a_h

enum Lab2ASignals {
	ENCODER_UP = Q_USER_SIG,
	ENCODER_DOWN,
	ENCODER_CLICK,
	MODE_1,
	MODE_2,
	MODE_3,
	MODE_4,
	MODE_5,
	START

};


extern struct Lab2ATag AO_Lab2A;


void Lab2A_ctor(void);
void GpioHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);
extern volatile int mode;

extern int spectro;

#endif  
