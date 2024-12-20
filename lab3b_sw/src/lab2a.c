/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "xil_printf.h"
#include "lab2a.h"
#include "lcd.h"
#include "fft.h"


typedef struct Lab2ATag  {               //Lab2A State machine
	QActive super;
}  Lab2A;

/* Setup state machines */
/**********************************************************************/
static QState Lab2A_initial (Lab2A *me);
static QState Lab2A_overall (Lab2A *me );
static QState Lab2A_main      (Lab2A *me);
static QState Lab2A_debug  (Lab2A *me);
static QState Lab2A_debugMain  (Lab2A *me);
static QState Lab2A_debug1  (Lab2A *me);
static QState Lab2A_debug2  (Lab2A *me);


/**********************************************************************/


Lab2A AO_Lab2A;

const int max_vol = 184;
const int min_vol = 56;
volatile int current_vol = 66;
volatile int mode = 1;

int spectro = 0;


void Lab2A_ctor(void)  {
	Lab2A *me = &AO_Lab2A;
	QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}


QState Lab2A_initial(Lab2A *me) {
	xil_printf("\n\rInitialization\r\n");
    return Q_TRAN(&Lab2A_overall);
}

QState Lab2A_overall(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("On\r\n");
		}
		case Q_INIT_SIG: {
			return Q_TRAN(&Lab2A_main);
		}
	}
	
	return Q_SUPER(&QHsm_top);
}

QState Lab2A_main(Lab2A *me){

	switch (Q_SIG(me)) {
			case Q_ENTRY_SIG: { //w
				drawScreen();
				drawMainInitial();
				spectro = 0;
				//xil_printf("%d\r\n",FFTON);
				if(FFTON){
					fftRunner();
				}
				return Q_HANDLED();
			}
			case START:{
				FFTON = 1;
				fftRunner();
				return Q_HANDLED();
			}

			case MODE_1:
			case MODE_2:
			case MODE_3:
			case MODE_5:
			case MODE_4: {
				//transition to debg main
				return Q_TRAN(&Lab2A_debugMain);

			}


		}


	return Q_SUPER(&Lab2A_overall);


}

QState Lab2A_debug(Lab2A *me){ // super state for debugging
	switch(Q_SIG(me)) {

		case Q_ENTRY_SIG: {
			//nothing for now
			//make sure fft not running
		}
		case Q_INIT_SIG: {
				return Q_TRAN(&Lab2A_debugMain);
		}
		case MODE_1: {
			//transition to lab2A_main
			FFTON = 1;
			return Q_TRAN(&Lab2A_main);
		}
	}

	return Q_SUPER(&Lab2A_overall);

}

QState Lab2A_debugMain(Lab2A *me){ //main screen - will have instructions on it
	switch(Q_SIG(me)) {

		case Q_ENTRY_SIG: {
			//draw main debug screen
			drawScreen();
			drawDebugMain();
			spectro = 0;
			return Q_HANDLED();
		}case MODE_2: {
			//transition to debug 1
			return Q_TRAN(&Lab2A_debug1);
		}case MODE_3: {
			FFTON = 1;
			spectro = 1;
			//transition to debug 2
			return Q_TRAN(&Lab2A_debug2);
		}
	}

	return Q_SUPER(&Lab2A_debug);

}



QState Lab2A_debug1(Lab2A *me){ //Histogram state
	switch(Q_SIG(me)) {

		case Q_ENTRY_SIG: {
			//draw debug 1
			drawScreen();
			drawHisto();
			return Q_HANDLED();
		}
		case MODE_1:
		case MODE_2:
		case MODE_3:
		case MODE_4:
		case MODE_5: {
			//transition to main debug
			return Q_TRAN(&Lab2A_debugMain);
		}

	}

	return Q_SUPER(&Lab2A_debug);
}

QState Lab2A_debug2(Lab2A *me){ //spectrogram state
	switch(Q_SIG(me)) {

		case Q_ENTRY_SIG: {
			//draw debug 2
			drawScreen();
			drawSpectroEntry();
			fftRunner();
			return Q_HANDLED();
		}
		case MODE_1:
		case MODE_2:
		case MODE_3:
		case MODE_4:
		case MODE_5: {
			//transition to debug main
			row = 290;
			return Q_TRAN(&Lab2A_debugMain);
		}
	}
	return Q_SUPER(&Lab2A_debug);

}





