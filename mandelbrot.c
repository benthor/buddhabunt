#include <stdio.h>
#include <stdlib.h>
//#include <complex.h>
#include "SDL/SDL.h"

#define MAXITER 1000000
#define SQR_ABS_MAX 7
#define WIDTH 1400
#define HEIGHT 1050
#define BUFFSIZE (WIDTH*HEIGHT)
#define	CURRSIZE (MAXITER*2)
#define BPP 4
#define DEPTH 32
#define IMG_MIN -2
#define IMG_MAX 2
#define REAL_MIN -2
#define REAL_MAX 1

#define X_ZOOM (((float)IMG_MAX-IMG_MIN)/WIDTH)
#define Y_ZOOM (((float)REAL_MAX-REAL_MIN)/HEIGHT)

#define NEXT_REAL(curr_real, curr_img, point_real) (curr_real*curr_real - curr_img*curr_img + point_real)
#define NEXT_IMAG(curr_real, curr_img, point_img) (2*curr_real*curr_img + point_img)

#define X2IMG(x) (IMG_MIN + x*X_ZOOM)
#define Y2REAL(y) (REAL_MIN + y*Y_ZOOM)

#define IMG2X(img) ((int)((img-IMG_MIN)/X_ZOOM)%WIDTH)
#define REAL2Y(real) ((int)((real-REAL_MIN)/Y_ZOOM)%HEIGHT)

#define SQR_ABS(x,y) (x*x + y*y)

#define RAND_X (rand()%WIDTH)
#define RAND_Y (rand()%HEIGHT)

/*
//this stuff is way too damn slow
//requires -lm to compile
complex float iterate_step(complex float start, complex float cpoint) {
	float start_real = creal(start);
	float start_img = cimag(start);
	float cpoint_real = creal(cpoint);
	float cpoint_img = cimag(cpoint);
	return (start_real*start_real - start_img*start_img + cpoint_real) 
		+ (2*start_real*start_img + cpoint_img)*I;
}
*/


int iterate_point(float c_real, float c_img, float max_square_absolute, int max_iteration) {
	float square_absolute = 0;
	int iteration = 0;
	//complex float cpoint = c_real + c_img * I;
	//complex float current = 0 + 0*I;
	float real = 0;
	float img = 0;


	float real_tmp, img_tmp;

	while ( square_absolute <= max_square_absolute && iteration < max_iteration ) {
		real_tmp = NEXT_REAL(real,img,c_real);
		img_tmp = NEXT_IMAG(real,img,c_img);
		img = img_tmp; real = real_tmp;
		
		// slower:
		//current = creal(current)*creal(current) - cimag(current)*cimag(current) + creal(cpoint)
		//	  + (2*creal(current)*cimag(current) + cimag(cpoint))*I;
		// slowest:
		//current = iterate_step(current, cpoint);

		iteration++;
	
		// fastest:
		square_absolute = SQR_ABS(real,img);
		// slower:
		//square_absolute = creal(current)*creal(current) + cimag(current)*cimag(current);
		// slowest:
		//square_absolute = cabs(current); 
	}

	return iteration;
}



void setPixel(SDL_Surface* screen, int x, int y, float shade) {
	Uint32* pixmem32; 
	
	pixmem32 = (Uint32*) screen->pixels + x + y*screen->w; 

	// TODO: check out if we could do cool stuff with an alpha channel here as well
	*pixmem32 = SDL_MapRGB(screen->format, (Uint8)(shade*255), (Uint8)(shade*255), (Uint8)(shade*255));
	//fprintf(stderr, "after\n");
}


void trace_point(SDL_Surface* screen, float real, float img, int current_iteration, int iteration_limit) {
	int sqr_abs = 0;

	//FIXME, maybe don't hardcode maximum square absolute here
	if (IMG2X(img) > 0 && REAL2Y(real) > 0 && sqr_abs <= 7 && current_iteration++ < iteration_limit) {
		setPixel(screen, IMG2X(img), REAL2Y(real), 0.5);
		SDL_Flip(screen);
		trace_point(screen, NEXT_REAL(real,img,0), NEXT_IMAG(real,img,0), current_iteration, iteration_limit);
	}
}


void print_array(SDL_Surface* screen, int* a, int l) {
	int i,x,y,max=0;
	for (i=0; i<l; i++) {
		if (a[i] > max) max = a[i];
	}
	for (i=0; i<l; i++) {
		setPixel(screen, i%WIDTH, i/WIDTH, (float)a[i]/max);
	}
}




void random_buddha_plane(SDL_Surface* screen, int min_iter, int max_iter, int maxpoints) {

	int counter = 0;
	int x,y;
	float curr_real;
	float curr_img;
	float point_real, point_img, next_real, next_img;
	int tmp;

	static int buffer[BUFFSIZE];
	static int current[CURRSIZE];
	
	int i,j;

	for (i=0; i<BUFFSIZE; buffer[i++]=0);
	for (i=0; i<CURRSIZE; current[i++]=0);

	while (counter < maxpoints) {
		x = RAND_X;
		y = RAND_Y;
		curr_real = 0;
		curr_img  = 0;
		point_real=Y2REAL(y);
		point_img=X2IMG(x);
		//tmp = iterate_point(point_real, point_img, 7, max_iter);

		current[0] = 0;
		current[1] = 0;
		i = 1;
		fprintf(stderr, "before->");
		while (i<(CURRSIZE-2)) {
			if (SQR_ABS(current[i-1],current[i]) >= SQR_ABS_MAX) {
				if (i>min_iter) {
					j = 0;
					while (j < i) {
						// MESSY
						//fprintf(stderr, "real: %d, imag: %d\n", current[j], current[j+1]);
						if (REAL2Y(current[j-1]) >= 0 && IMG2X(current[j]) >= 0) {
							buffer[IMG2X(current[j])+WIDTH*REAL2Y(current[j-1])]++;
						}
						j+=2;
					}
					fprintf(stderr, "after\n");
					if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
					print_array(screen, buffer, BUFFSIZE);
					SDL_Flip(screen);
					counter++;
				}
				break;
			}
			//I know this is messy
			current[i+1] = NEXT_REAL(current[i-1],current[i],point_real);
			current[i+2] = NEXT_IMAG(current[i-1],current[i],point_img);
			i+=2;
		}
		
		/*if (tmp >= min_iter && tmp < max_iter) {
			counter++;
			setPixel(screen, x, y, 0.5);
			while (SQR_ABS(curr_real, curr_img) < 7 && IMG2X(curr_img) >= 0 && REAL2Y(curr_real) >= 0) {
				//fprintf(stderr,"x = %i, y = %i\n", IMG2X(curr_img), REAL2Y(curr_real));
				buffer[IMG2X(curr_img)+WIDTH*REAL2Y(curr_real)]++;
				//setPixel(screen, IMG2X(curr_img), REAL2Y(curr_real), 1);
				next_real = NEXT_REAL(curr_real, curr_img, point_real);
				next_img  = NEXT_IMAG(curr_real, curr_img, point_img);
				curr_real = next_real;
				curr_img  = next_img;
			}
			
		}*/
	}
	//SDL_Flip(screen);
}

void iterate_plane(int iteration, SDL_Surface* screen) {
	int x,y;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}

	for (y=0; y<HEIGHT; y++){
		for (x=0; x<WIDTH; x++) {
			setPixel(screen, x, y, (float)(iterate_point(Y2REAL(y), X2IMG(x), 7, iteration))/iteration);
		}
	}
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}



int main(int argc, char* argv[]) {

	SDL_Surface* screen;
	SDL_Event event;

	int keypress = 0;
	int iteration = 1000;

	SDL_RWops* file;
	file = SDL_RWFromFile("output.bmp", "wb");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init video: %s\n", SDL_GetError());
		return 1;
	}

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE|SDL_HWSURFACE))) {
		fprintf(stderr, "Unable to set %ix%i video: %s\n", WIDTH, HEIGHT, SDL_GetError());
		SDL_Quit();
		return 1;
	}

	//first iteration
	//iterate_plane(iteration++, screen);
	random_buddha_plane(screen, 2000, 10000, WIDTH*HEIGHT/100);	
	SDL_SaveBMP_RW(screen, file, 1);
	//SDL_Quit();
	//return 0;
	
	while(1) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					fprintf(stderr, "quitting!\n");
					SDL_SaveBMP_RW(screen, file, 1);
					SDL_Quit();
					return 0;
					break;
				case SDL_KEYDOWN:
					iterate_plane(iteration++, screen);
					break;
				case SDL_MOUSEBUTTONDOWN:
					//iterate_plane(iteration++, screen);
					fprintf(stderr, "mouse x:%i, mouse y:%i\n", event.button.x, event.button.y);
					trace_point(screen, Y2REAL(event.button.y), X2IMG(event.button.x), 0, 10);
					break;
			}
		}
	}

	fprintf(stderr, "have reached the end somehow\n");

	return 0;

}
