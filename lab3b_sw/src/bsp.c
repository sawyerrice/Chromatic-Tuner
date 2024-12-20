/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xtmrctr.h"
#include <unistd.h>
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"
#include "xil_printf.h"


#include <stdio.h>
#include "xil_cache.h"
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xbasic_types.h>
#include <xil_assert.h>
#include "bsp.h"

#include <math.h>

#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include <inttypes.h>

#include "xintc.h"

#include <unistd.h>

#include <math.h>

/*****************************/

/* Define all variables and Gpio objects here  */

#define GPIO_CHANNEL1 1


void debounceInterrupt(); // Write This function

// Create ONE interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

XIntc sys_intc;
static XGpio BtnGpio;
static XGpio EncGpio;
int enc;
int state = 0;
int pinState = 0;

static XTmrCtr timer;
uint32_t Control;

static XGpio dc;
static XSpi spi;
XSpi_Config *spiConfig;	/* Pointer to Configuration data */
volatile int timerTrigger = 0;
//#define SAMPLES 128 // AXI4 Streaming Data FIFO has size 512

int reprintBase = 1;

// for 256 samples, accurate until 300 hz, all 0 below, bin spacing about 300 hz
#define CLOCK 100000000.0 //clock speed
#define SCALE_FACTOR (3.3 / 67108864.0)

int int_buffer[4096];
static float q[4096];
static float w[4096];

float hist[4096];

int displayFrequency = 0;
volatile int baseFrequency = 440;

int size = 0;





int octave = 0;
int note = 0;
int cents = 0;
int prevCents = 0;
int sampleFreq = 0;

int factor = 1;








int FFTON = 0;


void drawScreen() {
	setXY(0, 0, 239, 319);
	setColor(0, 150, 200);
	fillRect(0,0, 240, 320);
	//drawTriangles(0, 0, 239, 319);
}

/*..........................................................................*/

void BSP_init(void) {


	buildTable();
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

	/*
	 * Initialize the interrupt controller driver so that
	 * it is ready to use, specify the device ID that is generated in
	 * xparameters.h
	 */

	//u32 controlReg;

	XStatus Status;

	// Initialize the interrupt controller

	Status = XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Interrupt controller initialization failed...\r\n");
		return;
	}
	xil_printf("Interrupt controller initialized!\r\n");

	XGpio_Initialize(&BtnGpio, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	XGpio_Initialize(&EncGpio, XPAR_AXI_GPIO_JD_DEVICE_ID);

	//XGPIO_Initialize(&EncGpio, XPAR_AXI_GPIO_BTN_DEVICE_ID);

	// Connect interrupt handlers for the button and encoder
	Status = XIntc_Connect(&sys_intc,
							XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
							(XInterruptHandler) ButtonHandler, &BtnGpio);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to connect Button interrupt handler...\r\n");
		return;
	}

	Status = XIntc_Connect(&sys_intc,
							XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_JD_IP2INTC_IRPT_INTR,
							(XInterruptHandler) EncoderHandler, &EncGpio);
	if (Status != XST_SUCCESS) {
		xil_printf("Failed to connect Encoder interrupt handler...\r\n");
		return;
	}



	 Status = XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
	 if (Status != XST_SUCCESS) {
	 		xil_printf("Initialize timer fail!\n");
	 }

	 Control = XTmrCtr_GetOptions(&timer, 0) | XTC_CAPTURE_MODE_OPTION | XTC_INT_MODE_OPTION;

	 XTmrCtr_SetOptions(&timer, 0, Control);


//	/*
//	 * Initialize the GPIO driver so that it's ready to use,
//	 * specify the device ID that is generated in xparameters.h
//	 */
	Status = XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	if (Status != XST_SUCCESS)  {
		xil_printf("Initialize GPIO dc fail!\n");
	}

	/*
	 * Set the direction for all signals to be outputs
	 */
	XGpio_SetDataDirection(&dc, 1, 0x0);

	/*
	 * Initialize the SPI driver so that it is  ready to use.
	 */
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	if (spiConfig == NULL) {
		xil_printf("Can't find spi device!\n");
	}

	Status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
	}

	/*
	 * Reset the SPI device to leave it in a known good state.
	 */
	XSpi_Reset(&spi);

	/*
	 * Setup the control register to enable master mode
	 */
	u32 controlReg;

	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	// Select 1st slave device
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	initLCD();

	clrScr();

	drawScreen();
//
//	// Press Knob
//
//	// Twist Knob
		
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	//xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	XStatus Status;
	Status = XST_SUCCESS;

	//XTmrCtr_Start(&axi0ctr,0);
	//XTmrCtr_Start(&timer, 0);

	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_JD_IP2INTC_IRPT_INTR);

	Status = XIntc_Start(&sys_intc, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		xil_printf("Start interrupt controller fail!\n");
	}

	XGpio_InterruptEnable(&EncGpio, 1);
	XGpio_InterruptGlobalEnable(&EncGpio);

	XGpio_InterruptEnable(&BtnGpio, 1);
	XGpio_InterruptGlobalEnable(&BtnGpio);

	microblaze_register_handler((XInterruptHandler) XIntc_DeviceInterruptHandler,
									(void*) XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);


	microblaze_enable_interrupts();
	xil_printf("Interrupts enabled!\r\n");

	QActive_postISR((QActive *)&AO_Lab2A, START);

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}


void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();

   /* unlock interrupts */

    {
    	// Write code to increment your interrupt counter here.
    	// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM



// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


    }
}

/* Q_onAssert is called only when the program encounters an error*/
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* name of the file that throws the error */
    (void)line;                                   /* line of the code that throws the error */
    QF_INT_LOCK();
    printDebugLog();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/
void ButtonHandler(void *CallbackRef) {

	//XTmrCtr_Reset(&axi0ctr, 0);

	XGpio *GpioPtr = (XGpio *)CallbackRef;

	unsigned int btn = XGpio_DiscreteRead(&BtnGpio, 1);



	//xil_printf("FFTON INTERRUPT HANDLER %d\r\n",FFTON);

	//xil_printf("FFTON INTERRUPT HANDLER %d\r\n",btn);


	if(btn){


		if(FFTON == 1){
			  FFTON = 0;

			}

		if(btn == 1){
			mode = 1;
			QActive_postISR((QActive *)&AO_Lab2A, MODE_1);
		}
		else if(btn == 2){
			mode = 2;
			QActive_postISR((QActive *)&AO_Lab2A, MODE_2);
		}
		else if(btn == 4){
			mode = 3;
			QActive_postISR((QActive *)&AO_Lab2A, MODE_3);
		}
		else if(btn == 8){
			mode = 4;
			//FFTON = FFTON ^ 1;
			QActive_postISR((QActive *)&AO_Lab2A, MODE_4);
		}
		else {
			mode = 5;
			QActive_postISR((QActive *)&AO_Lab2A, MODE_5);
		}
	}


	usleep(9000);
	XGpio_InterruptClear(GpioPtr, 1);



}

void EncoderHandler(void *CallbackRef) {
	//XTmrCtr_Reset(&axi0ctr, 0);

	XGpio *GpioPtr = (XGpio *)CallbackRef;

	enc = XGpio_DiscreteRead(&EncGpio, 1); //enc = 0 = pin1 down, pin2 down, button down

	if(enc >= 4){
		debounceInterrupt();
	} else{
		pinState = enc;
		debounceTwistInterrupt();
	}

	XGpio_InterruptClear(GpioPtr, 1);
}

//void TwistHandler(void *CallbackRef) {
//	//XGpio_DiscreteRead( &twist_Gpio, 1);
//
//}

void debounceTwistInterrupt(){
	// Read both lines here? What is twist[0] and twist[1]?
	// How can you use reading from the two GPIO twist input pins to figure out which way the twist is going?
	switch(state){
		case 0:
			switch(pinState){
				case 1: //pin 2 went down
					state = 4;
					break;
				case 2: // pin 1 down
					state = 1;
					break;
				default:
					break;
			}
			break;

		case 1:
			switch(pinState){
				case 3:
					state = 0;
					break;
				case 0:
					state = 2;
					break;
				default:
					break;
		}
			break;

		case 2:
			switch(pinState){
				case 2:
					state = 1;
					break;
				case 1:
					state = 3;
					break;
				default:
					break;
			}
			break;

		case 3:
			switch(pinState){
				case 3:
					state = 0;

					if(baseFrequency > 420){
						baseFrequency--;
					}
					reprintBase = 1;

					//QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN);
					return;
					break;
				case 0:
					state = 2;
					break;
				default:
					break;
			}
			break;

		case 4:
			switch(pinState){
				case 3:
					state = 0;
					break;
				case 0:
					state = 5;
					break;
				default:
					break;
			}
			break;

		case 5:
			switch(pinState){
				case 1:
					state = 4;
					break;
				case 2:
					state = 6;
					break;
				default:
					break;
			}
			break;

		case 6:
			switch(pinState){
				case 0:
					state = 5;
					break;
				case 3:
					state = 0;

					if(baseFrequency < 460){
						baseFrequency++;
					}
					reprintBase = 1;
					//QActive_postISR((QActive *)&AO_Lab2A, ENCODER_UP);
					return;
					break;
				default:
					break;
			}
			break;
	default:
		break;
	}
}

void debounceInterrupt() {
	usleep(500);
	QActive_postISR((QActive *)&AO_Lab2A, ENCODER_CLICK);
	XGpio_InterruptClear(&EncGpio, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}

static inline void read_fsl_values(float* q, int n) {
   int i;
   unsigned int x;
   stream_grabber_start();
   stream_grabber_wait_enough_samples(1);

   for(i = 0; i < n; i++) {
      int_buffer[i] = stream_grabber_read_sample(i);
      // xil_printf("%d\n",int_buffer[i]);
      x = int_buffer[i];
      q[i] = x*SCALE_FACTOR; // 3.3V and 2^26 bit precision.


   }
}

void decimate(int factor, int size){
//decimating effectively decreases sample frequency, which decreases bin spacing, and can increase accuracy
	int idx = 0;
	for(int i = 0; i < size; i+= factor){
		float sum = 0;
		for(int j = i; j < factor + i; j++){
			sum += q[j];
			q[j] = 0;
		}
		float avr = sum/ factor;
		q[idx] = avr;
		idx++;
	}
}

void getNoteAndOctave(float frequency, int baseFrequency) {

	const float expc0 = -1*((float)9/12) - 4;

    float n = 12 * log2(frequency / baseFrequency);

    note = ((int)((round)(n))) % 12;
    if (note < 0) note += 12;

    float c0 = baseFrequency *pow(2, expc0);

    if(note == 3){
    	octave = (round)(log2(frequency / c0));
    }else{
    	octave = (int)(log2(frequency / c0));
    }

    float frac = n - floor(n);
    if(frac > .5){
    	cents = -1*(int)(100*(1-frac));
    }else{
    	cents = (int)(100*frac);
    }

}

//void zeroPadding(int newSize){
//	//factor by which to expand.... 2,4,8,16...
//	for(int i = newSize-1; i >= (SAMPLES/factor); i--){// is index in q
//		q[i] = 0.0;
//	}
////	xil_printf("FACTOR: %d\r\n", expansion);
////	xil_printf("PREVIOUS SIZE: %d\r\n", SAMPLES/factor);
////	for(int i = 0; i < 128; i++){// is index in q
////			printf("%f, ",q[i]);
////		}
////	printf("\n");
//
//
//}




void fftRunner() {
   const float sample_f = 100*1000*1000/2048.0;

   int l;
   int ticks; //used for timer
   float frequency = 0;
   float tot_time; //time to run program
   int ct = 0;
   reprintBase  = 1;
   int prevFrequency = 0;

   int average_counter = 0;
   int sum_frequencies = 0;

   while(FFTON) {

	   XTmrCtr_Start(&timer, 0);
	   frequency = 0;
	   factor = 1;

	   while(frequency < 1){//or find point at which frequency = 0

	   for(l=0;l<SAMPLES;l++){
			   w[l]=0;
	   }
	   read_fsl_values(q, SAMPLES);
	   decimate(4, SAMPLES);
	   frequency= fft(q,w,SAMPLES/4,log2(SAMPLES/4),sample_f/4);
	   }

	   factor = pow(2,(round)(log2(sample_f/(4*frequency))+.5));

	   for(int j =0; j < 4096/4; j++){
		   hist[j] = new_[j];
	   }
	   size = SAMPLES/4;
	   sampleFreq = sample_f/4;

	   //gets benchmark

	   float sum = 0.0;
	   read_fsl_values(q, SAMPLES);
	   decimate(factor, SAMPLES);
	   for(l=0;l<SAMPLES;l++){
				  w[l]=0;
		}

		frequency= fft(q,w,(SAMPLES/factor),log2((SAMPLES/factor)),sample_f/(factor));


        int prevNote = note;
        int prevOctave = octave;

        if((frequency <= (prevFrequency+10)) && (frequency >= (prevFrequency-10))){
			sum_frequencies += frequency;
			average_counter++;
			displayFrequency = (sum_frequencies / average_counter);

		}
		else {
			sum_frequencies = 0;
			average_counter = 0;
		}


        prevFrequency = displayFrequency;

        prevCents = cents;

        if(!spectro){

			if(frequency != 0){
				getNoteAndOctave(frequency, baseFrequency);
			}

			displayFrequency = (int)(frequency+.5);

			if(prevNote != note || prevOctave != octave || ct == 0){
				drawNoteInfo();
				drawCentText();
				drawCents();
				drawFrequency();
			}else if(prevCents != cents){
				drawCentText();
				drawCents();
				drawFrequency();
			}

			if(reprintBase){
				drawBase();
				reprintBase = 0;
			}

        }else{
        	drawSpectro();
        }



	    ticks=XTmrCtr_GetValue(&timer, 0);
		XTmrCtr_Stop(&timer, 0);
		tot_time=ticks/CLOCK;

		//xil_printf("program time: %dms \r\n",(int)(1000*tot_time));


		ct++;
   }



}

