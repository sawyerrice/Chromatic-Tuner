/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h

/* bsp functions ..........................................................*/
#include "xtmrctr.h"

#define SAMPLES 4096
#define M 12

void BSP_init(void);
void ISR_gpio(void);
void ISR_timer(void);
void ButtonHandler(void *);
void EncoderHandler(void *);
void debounceTwistInterrupt();

extern volatile int mode;
extern int FFTON;
extern int displayFrequency;
extern volatile int baseFrequency;

extern int note;
extern int octave;
extern int cents;
extern int prevCents;
extern int size;
extern int sampleFreq;
extern int factor;

extern float hist[4096];








void printDebugLog(void);

#define BSP_showState(prio_, state_) ((void)0)


#endif                                                             /* bsp_h */


