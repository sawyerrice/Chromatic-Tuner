/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
 UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
 Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved

 This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
 and RGB_GLCD libraries for Arduino and chipKit. As the number of
 supported display modules and controllers started to increase I felt
 it was time to make a single, universal library as it will be much
 easier to maintain in the future.

 Basic functionality of this library was origianlly based on the
 demo-code provided by ITead studio (for the ITDB02 modules) and
 NKC Electronics (for the RGB GLCD module/shield).

 This library supports a number of 8bit, 16bit and serial graphic
 displays, and will work with both Arduino, chipKit boards and select
 TI LaunchPads. For a full list of tested display modules and controllers,
 see the document UTFT_Supported_display_modules_&_controllers.pdf.

 When using 8bit and 16bit display modules there are some
 requirements you must adhere to. These requirements can be found
 in the document UTFT_Requirements.pdf.
 There are no special requirements when using serial displays.

 You can find the latest version of the library at
 http://www.RinkyDinkElectronics.com/

 This library is free software; you can redistribute it and/or
 modify it under the terms of the CC BY-NC-SA 3.0 license.
 Please see the included documents for further information.

 Commercial use of this library requires you to buy a license that
 will allow commercial use. This includes using the library,
 modified or not, as a tool to sell products.

 The license applies to all part of the library including the
 examples and tools supplied with the library.
 */

#include "lcd.h"
#include "bsp.h"
#include "fft.h"
#include <math.h>

// Global variables
int fch;
int fcl;
int bch;
int bcl;
struct _current_font cfont;

char debugStr[]= "Debug: 1";
//char mainStr[]= "AB";



float shrink[4096];
int scaleText = 3;
int scaleNote = 2;

int globalMax = 0;

// Write command to LCD controller
void LCD_Write_COM(char VL) {
	Xil_Out32(SPI_DC, 0x0);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL) {
	Xil_Out32(SPI_DC, 0x01);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Initialize LCD controller
void initLCD(void) {
	int i;

	// Reset
	LCD_Write_COM(0x01);
	for (i = 0; i < 500000; i++)
		; //Must wait > 5ms

	LCD_Write_COM(0xCB);
	LCD_Write_DATA(0x39);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x34);
	LCD_Write_DATA(0x02);

	LCD_Write_COM(0xCF);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0XC1);
	LCD_Write_DATA(0X30);

	LCD_Write_COM(0xE8);
	LCD_Write_DATA(0x85);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x78);

	LCD_Write_COM(0xEA);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xED);
	LCD_Write_DATA(0x64);
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0X12);
	LCD_Write_DATA(0X81);

	LCD_Write_COM(0xF7);
	LCD_Write_DATA(0x20);

	LCD_Write_COM(0xC0);   //Power control
	LCD_Write_DATA(0x23);  //VRH[5:0]

	LCD_Write_COM(0xC1);   //Power control
	LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0]

	LCD_Write_COM(0xC5);   //VCM control
	LCD_Write_DATA(0x3e);  //Contrast
	LCD_Write_DATA(0x28);

	LCD_Write_COM(0xC7);   //VCM control2
	LCD_Write_DATA(0x86);  //--

	LCD_Write_COM(0x36);   // Memory Access Control
	LCD_Write_DATA(0x48);

	LCD_Write_COM(0x3A);
	LCD_Write_DATA(0x55);

	LCD_Write_COM(0xB1);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x18);

	LCD_Write_COM(0xB6);   // Display Function Control
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x82);
	LCD_Write_DATA(0x27);

	LCD_Write_COM(0x11);   //Exit Sleep
	for (i = 0; i < 100000; i++)
		;

	LCD_Write_COM(0x29);   //Display on
	LCD_Write_COM(0x2c);

	//for (i = 0; i < 100000; i++);

	// Default color and fonts
	fch = 0xFF;
	fcl = 0xFF;
	bch = 0x00;
	bcl = 0x00;
	setFont(SmallFont);
}

// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2) {
	LCD_Write_COM(0x2A);
	LCD_Write_DATA(x1 >> 8);
	LCD_Write_DATA(x1);
	LCD_Write_DATA(x2 >> 8);
	LCD_Write_DATA(x2);
	LCD_Write_COM(0x2B);
	LCD_Write_DATA(y1 >> 8);
	LCD_Write_DATA(y1);
	LCD_Write_DATA(y2 >> 8);
	LCD_Write_DATA(y2);
	LCD_Write_COM(0x2C);
}

// Remove boundry
void clrXY(void) {
	setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	fch = (r & 0x0F8) | g >> 5;
	fcl = (g & 0x1C) << 3 | b >> 3;
}

// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	bch = (r & 0x0F8) | g >> 5;
	bcl = (g & 0x1C) << 3 | b >> 3;
}

// Clear display
void clrScr(void) {
	// Black screen
	setColor(0, 0, 0);

	fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

void clrVol(int curr) {
	setColor(0,120,0);
	fillRect(56, 50, 184, 82);
	drawTriangles(56, 50, 184, 82);
}

void redrawVol(int curr) {
	setColor(255, 255, 255);
	fillRect(56, 50, curr, 82);
}

void increaseVol(int curr){
	setColor(255,255,255);
	fillRect(curr-2, 50, curr, 82);
}

void decreaseVol(int curr){
	setColor(0,120,0);
	fillRect(curr, 50, curr+2, 82);
	drawTriangles(curr, 50, curr+2, 82);
}

// Draw horizontal line
void drawHLine(int x, int y, int l) {
	int i;

	if (l < 0) {
		l = -l;
		x -= l;
	}

	setXY(x, y, x + l, y);
	for (i = 0; i < l + 1; i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

// Fill a rectangular 
void fillRect(int x1, int y1, int x2, int y2) {
	int i;

	if (x1 > x2)
		swap(int, x1, x2);

	if (y1 > y2)
		swap(int, y1, y2);

	setXY(x1, y1, x2, y2);
	for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

// Select the font used by print() and printChar()
void setFont(u8* font) {
	cfont.font = font;
	cfont.x_size = font[0];
	cfont.y_size = font[1];
	cfont.offset = font[2];
	cfont.numchars = font[3];
}

// Print a character

//void clearString(){// used for clearing mode: x, not especially useful now
//	int x = 60;
//	int y = 100;
//
//	setColor(0, 120, 0);
//	fillRect(x, y, x + (7*cfont.x_size) - 1, y + cfont.y_size);
//	drawTriangles(x, y, x + (7*cfont.x_size) - 1, y + cfont.y_size);
//}

void printChar(u8 c, int x, int y) {
	u8 ch;
	int i, j, pixelIndex;

	setXY(x, y, x + cfont.x_size - 1, y + cfont.y_size - 1);

	pixelIndex = (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
	for (j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
		ch = cfont.font[pixelIndex];
		for (i = 0; i < 8; i++) {
			if ((ch & (1 << (7 - i))) != 0) {
				LCD_Write_DATA(fch);
				LCD_Write_DATA(fcl);
			} else {
				LCD_Write_DATA(bch);
				LCD_Write_DATA(bcl);
			}
		}
		pixelIndex++;
	}

	clrXY();
}

void printCharBig(u8 c, int x, int y, int scale) {
    u8 ch;
    int i, j, k, l, pixelIndex;

    // Set scaled dimensions


    int scaledWidth = cfont.x_size * scale;
    int scaledHeight = cfont.y_size * scale;

    setXY(x, y, x + scaledWidth - 1, y + scaledHeight - 1);

    pixelIndex = (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
    for (j = 0; j < cfont.y_size; j++) { // Iterate over rows of the character
        for (int rowRepeat = 0; rowRepeat < scale; rowRepeat++) { // Scale rows
            for (int byteIndex = 0; byteIndex < (cfont.x_size >> 3); byteIndex++) {
                ch = cfont.font[pixelIndex + byteIndex];
                for (i = 0; i < 8; i++) { // Iterate over bits in the byte
                    for (int colRepeat = 0; colRepeat < scale; colRepeat++) { // Scale columns
                        if ((ch & (1 << (7 - i))) != 0) {
                            LCD_Write_DATA(fch);
                            LCD_Write_DATA(fcl);
                        } else {
                            LCD_Write_DATA(bch);
                            LCD_Write_DATA(bcl);
                        }
                    }
                }
            }
        }
        pixelIndex += (cfont.x_size >> 3); // Move to the next row in the font data
    }

    clrXY();
}


// Print string

void clrCents() {
	setColor(0,150,200);
	fillRect(70 ,180 , 170, 200);

}

void drawCents(){


	if(cents < 0 && prevCents < 0){
		//draw red left,
		int diff = cents - prevCents;

		if(diff < 0){ //cents < prevCents, so draw more
			setColor(168,13,13);
			fillRect(120+(cents), 180, 120+ (prevCents), 200);

		}else{
			//preveCents <cents
			setColor(0,150,200);
			fillRect(120+(prevCents), 180, 120+(cents), 200);
		}

		// x = 120 is center
	}else if(cents > 0 && prevCents > 0){
		int diff = cents - prevCents;

		if(diff < 0){
			setColor(0,150,200);
			fillRect(120+(cents), 180, 120 + (prevCents), 200);
		}else{

			setColor(17,143,49);
			fillRect(120+(prevCents), 180, 120+(cents), 200);
		}
	}else if(cents < 0){

		clrCents();

		setColor(168,13,13);
		fillRect(120+cents, 180, 120, 200);

	}else if (cents >= 0){

		clrCents();

		setColor(17,143,49);
		fillRect(120, 180, 120+cents, 200);

		}

	}


void redrawBigPrint(int x, int y){

	setColor(0, 150, 200);
	fillRect(20,60,230,120);

}
void lcdBigPrint(char *st, int x, int y, int scale) {
	int i = 0;
	while (*st != '\0')
		printCharBig(*st++, x + scale*cfont.x_size * i++, y,scale);
}

void lcdPrint(char *st, int x, int y) {


	int i = 0;
	while (*st != '\0')
		printChar(*st++, x + cfont.x_size * i++, y);
}

void intToCharArray(int num, char* buffer, int bufferSize) {
    snprintf(buffer, bufferSize, "%d", num);
}

void formatNoteAndOctave(const char* note, int octave, char* result) {
    // Format the note and octave into the result buffer
    sprintf(result, "%s%d", note, octave);
}

char textMainScreen[]= "TUNER";
char line[]= "=====";
char baseNote[]= "A4: ";
char writeFreq[]= "HZ: ";
char centText[]= "CTS:";
void drawMainInitial(){

	setFont(BigFont);
	setColor(0, 0, 0);
	setColorBg(0,150,200);
	lcdBigPrint(textMainScreen, 0, 20, 3);
	lcdBigPrint(line, 0, 65, 3);
	lcdPrint(baseNote, 0, 300);
	lcdPrint(writeFreq, 0, 280);
	lcdPrint(centText, 0, 260);



	drawHLine(69, 179, 102);
	drawHLine(69, 201, 102);

	fillRect(69, 179, 69, 201);

	fillRect(171, 179, 171, 201);

}

int prevNote = 0;

const char* notes[] = {"A", "A#", "B","C", "C#", "D", "D#", "E", "F", "F#", "G", "G#" };

void drawNoteInfo(){



	char formattedNote[10]; // Ensure enough space for the note and octave
	formatNoteAndOctave(notes[note], octave, formattedNote);

	prevNote = note;

	if(prevNote = 1 || note == 4 || note == 6 || note == 9 || prevNote ==11){
		setColor(0,150,200);
		fillRect(144,120,176,150);
	}
	setColor(0, 0, 0);
	setColorBg(0,150,200);

	lcdBigPrint(formattedNote, 80, 120, 2);


}

int prevfrequency = 0;


void drawFrequency(){


	char buffer[10]; // Make sure the buffer is large enough to hold the number and null terminator



	intToCharArray(displayFrequency, buffer, sizeof(buffer));

	int digits =0;
	if(prevfrequency >= 1000){
		digits = 4;
	}else if(prevfrequency  >= 100){
		digits = 3;
	}else if(prevfrequency > 10){
		digits = 2;
	}

	setColor(0,150,200);
	fillRect(64,280,48+(16*digits),300);


	setColor(0, 0, 0);
	setColorBg(0,150,200);

	lcdPrint(buffer, 48, 280);

	prevfrequency = displayFrequency;
}

char debugMain[]= "DEBUG MAIN";
char debugHistText[]= "<--HISTOGRAM";
char debugSpecText[]= "SPECTROGRAM-->";
char header[]= ">->->";
char header2[]= "<-<-<";
char debugRetText[]= "UP TO RETURN";


void drawDebugMain(int mode){
	setFont(BigFont);
	setColor(0, 0, 0);
	lcdBigPrint(header, 0, 10,3);
	lcdPrint(debugMain, 40, 80);
	lcdPrint(debugRetText, 25, 110);
	lcdPrint(debugHistText, 15, 150);
	lcdPrint(debugSpecText, 15, 190);
	lcdBigPrint(header2, 0, 270,3);

}


char debugHistWriting[]= "HISTOGRAM";

void drawDebugHist(){
	setFont(BigFont);
	setColor(0, 0, 0);
	lcdPrint(debugHistWriting, 60, 120);

}




void drawBase(){

	char buffer[10]; // Make sure the buffer is large enough to hold the number and null terminator




	intToCharArray(baseFrequency, buffer, sizeof(buffer));
	setColor(0, 0, 0);
		setColorBg(0,150,200);

	lcdPrint(buffer, 48, 300);
}


void drawCentText(){

	setColor(0,150,200);
	fillRect(64,260,110,275);

	char buffer[10]; // Make sure the buffer is large enough to hold the number and null terminator

	intToCharArray(cents, buffer, sizeof(buffer));

	setColor(0, 0, 0);
	setColorBg(0,150,200);



	lcdPrint(buffer, 64, 260);
}

void clrMode(void){
	setColor(0, 120, 0);
	drawTriangles(52, 94, 108, 106);
	//fillRect(52, 94, 108, 106);
}


volatile int row = 290;

void drawSpectroEntry(){

	setColor(0, 0, 255);
	fillRect(10,30,231,290);

}

void drawSpectro(){

	if(row < 30){
		row = 290;
		drawSpectroEntry();
	}

	float binSpacing = sampleFreq/SAMPLES;

	float min_value = ((sampleFreq)/size)*8;//size of a bin
	float max_value = sampleFreq/2;

	float histMax = 0;
	for (int i = 1; i < size; i++) {	    //min_value = fminf(min_value, hist[i]);
	    histMax = fmaxf(histMax, hist[i]);
	}

	// Calculate the logarithmic range
	float log_range = log10f(max_value) - log10f(min_value);


	float range = histMax/3;

	int writtenPixel = 0;
	float running_max = hist[0];
	float previous_pixel = -1.0;


	for (int i = 0; i < size/2; i++) {


	    float log_value = log10f(((sampleFreq)/size)*i) - log10f(min_value);
	    int y_pixel = (int)round((log_value / log_range) * 220); // Adjust for 0-based indexing


	    if(y_pixel != previous_pixel || i == (size/2)-1){

	    	if(running_max > range*2){
	    		    //print red
	    		    setColor(255, 0, 0);
	    	}else if(running_max > range/2){
	    		    //print green
	    		    setColor(0, 255, 0);

	    	}else{
	    		    //print blue
	    		    setColor(0, 0, 255);

	    	}

	    	running_max = hist[i];


	    	while(writtenPixel < previous_pixel){
	    		    drawHLine(230-(writtenPixel), row, 1);
	    		    writtenPixel++;
	    	}

	    	previous_pixel = y_pixel;


	    }else{
	    	if(hist[i] > running_max){
	    		running_max = hist[i];
	    	}
	    }


	}
	row--;


	//find proprotion to determine strong medium weak
}

void compressArray(float* array) {

}

void drawHisto() {


	setColor(255,255,255);
	fillRect(0,10,240,310);

	float nahwuznah[16];

	//compressArray(nahwuznah);

	int trueSamples = 512;

	int combination = trueSamples / 16;

		float sum = 0;

	int num = 0;
	float max = 0;
	int ct = 1;

		for(int i = 1; i < 512; i++){
			sum += hist[i];
			if( num ==  combination){
				nahwuznah[ct] = sum;

				if(sum > max){
					max = sum;
				}

				sum = 0;
				ct++;
				num = 0;
			}
			num++;
		}






	float multiplier = 220 / max;



	int offset = 5;


	for(int i=1; i <= 15; i++){


		setColor(0,200,45);
		fillRect(240 - (int)(nahwuznah[i]*multiplier),20*(i-1)+10 + offset, 240, 20*i+10);
	}


}



























void drawTriangles(int x1,int y1,int x2,int y2){

	setColor(255,0,0);
	int col = y1%40;
	//int row = x1%40;//where you are in the current triangle/ out of triangle


	int triangleWidth  = 40 - ((col+1)/2)*2;

	int spaceBetween = (40-triangleWidth);

	for(int i = y1; i <= y2; i++){

		if(i % 2 == 0 && i != y1){
			triangleWidth-=2;
			spaceBetween +=2;
			if(triangleWidth <= 0){
				triangleWidth = 40;
				spaceBetween = 0;
			}
		}

		int startTri =  (spaceBetween/2) + ((x1/40)*40);//pixel value for start of triangle on this row
		int endTri = startTri +(triangleWidth) -1;
		int inTri   = 0; // i,j is in the triangle

		int j  = x1;
		while(j <= x2){

			if((j >= startTri) && (j <= endTri)){
				inTri = 1;
			}


			if(inTri){
				fillRect(j, i, endTri, i);
				j = startTri+40;

				endTri += 40;
				startTri +=40;
			}else{
				if(x1 < startTri){
					j = startTri ;//start of next triangle
				}else{
					j = startTri+40;
					endTri += 40;
					startTri +=40;
				}
			}
		}
	}
}




