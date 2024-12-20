#include "fft.h"
#include "complex.h"
#include "trig.h"

#include <stdio.h>
#include "xil_cache.h"
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xbasic_types.h>
#include <xil_assert.h>
#include "bsp.h"

#include <xio.h>
#include "xtmrctr.h"
#include "fft.h"
#include "note.h"
#include "stream_grabber.h"
#include <inttypes.h>

#include "xintc.h"

#include <unistd.h>

#include <math.h>

float new_[4096];

static float new_im[4096];

XTmrCtr timer1;
XIntc intc;

static float cosTable[12][2048];
static float sinTable[12][2048];




void buildTable(){
	for(int j =0 ; j< 12; j++){//j = log_2(b), b = 2^j
		int k = 0;
		while(k < pow(2.0, j)){ // k
			cosTable[j][k] = cos(-PI* k/pow(2.0, j));
			sinTable[j][k] = sin(-PI* k/pow(2.0, j));
			k++;
		}
	}
}


float fft(float* q, float* w, int n, int m, float sample_f) {
	int a,b,r,d,e,c;
	int k,place;
	a=n/2;
	b=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;


	int bit_reversed_index;
	    float temp_real, temp_imag;

	    for (i = 0; i < n; i++) {
	        // Compute bit-reversed index
	        bit_reversed_index = 0;
	        for (j = 0; (1 << j) < n; j++) {
	            if (i & (1 << j)) {
	                bit_reversed_index |= (n >> (j + 1));
	            }
	        }

	        // Swap only if bit-reversed index > current index
	        if (i < bit_reversed_index) {
	            // Swap real parts
	            temp_real = q[i];
	            q[i] = q[bit_reversed_index];
	            q[bit_reversed_index] = temp_real;

	            // Swap imaginary parts
	            temp_imag = w[i];
	            w[i] = w[bit_reversed_index];
	            w[bit_reversed_index] = temp_imag;
	        }
	    }

	b=1;
	k=0;
	for (j=0; j<m; j++){
	//MATH
		for(i=0; i<n; i+=2){
			if (i%(n/b)==0 && i!=0)
				k++;

			real=mult_real(q[i+1], w[i+1], cosTable[j][k], sinTable[j][k]);
			imagine=mult_im(q[i+1], w[i+1],cosTable[j][k] , sinTable[j][k]);

			new_[i]=q[i]+real;
			new_im[i]=w[i]+imagine;
			new_[i+1]=q[i]-real;
			new_im[i+1]=w[i]-imagine;

		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END MATH

	//REORDER
		for (i=0; i<n/2; i++){
			new_[i]=q[2*i];
			new_[i+(n/2)]=q[2*i+1];
			new_im[i]=w[2*i];
			new_im[i+(n/2)]=w[2*i+1];
		}
		for (i=0; i<n; i++){
			q[i]=new_[i];
			w[i]=new_im[i];
		}
	//END REORDER
		b = b << 1; //**********
		k=0;
	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(n/2);i++) {
		new_[i]=q[i]*q[i]+w[i]*w[i];

		//new_[i]*=2;

		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}



	float s=sample_f/n; //spacing of bins

	frequency = (s)*place;


	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

//	xil_printf("HISTO \r\n");
//	for(int i = 0 ; i < n;i++){
//		printf("%f ", new_[i]);
//	}printf("\n");
//	xil_printf("\r\n");

	return frequency;
}
