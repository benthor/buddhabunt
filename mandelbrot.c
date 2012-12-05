#include <stdio.h>
#include <complex.h>
#include "SDL/SDL.h"

#define WIDTH 1024
#define HEIGHT 768
#define BPP 4
#define DEPTH 32
#define IMG_MIN = -2f;
#define IMG_MAX = 2f;
#define REAL_MIN = -2f;
#define REAL_MAX = 1f;


#define NEXT_REAL(curr_real, curr_img, point_real) (curr_real*curr_real - curr_img*curr_img + point_real)
#define NEXT_IMAG(curr_real, curr_img, point_img) (2*curr_real*curr_img + point_img)


//this stuff is way too damn slow
complex float iterate_step(complex float start, complex float cpoint) {
	float start_real = creal(start);
	float start_img = cimag(start);
	float cpoint_real = creal(cpoint);
	float cpoint_img = cimag(cpoint);
	return (start_real*start_real - start_img*start_img + cpoint_real) 
		+ (2*start_real*start_img + cpoint_img)*I;
}


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
		square_absolute = real*real + img*img;
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


void trace_point(SDL_Surface* screen, int x, int y, int current_iteration, int iteration_limit) {
}


void iterate_plane(int iteration, SDL_Surface* screen) {

	int pix_x, pix_y;
	float c_real, c_img;
	int w = screen->w;
	int h = screen->h;
	int x,y;

	float y_zoom = (REAL_MAX-REAL_MIN)/h;
	float x_zoom = (IMG_MAX-IMG_MIN)/w;


	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}

	for (y=0; y<h; y++){
		for (x=0; x<w; x++) {
			c_img = IMG_MIN + x*x_zoom;
			c_real = REAL_MIN + y*y_zoom;
			//if ( iteration == iterate_point(c_real, c_img, 7, iteration)) {
				//setPixel(screen, x, y, (1/(float)iteration));
				setPixel(screen, x, y, (float)(iterate_point(c_real, c_img, 7, iteration))/iteration);
			//} else {
				//setPixel(screen, x, y, 1);
			//}
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
	iterate_plane(iteration++, screen);
	fprintf(stderr, "quitting!\n");
	SDL_SaveBMP_RW(screen, file, 1);
	SDL_Quit();
	return 0;
	
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
					iterate_plane(iteration++, screen);
					fprintf(stderr, "mouse x:%i, mouse y:%i\n", event.button.x, event.button.y);
					break;
			}
		}
	}

	fprintf(stderr, "have reached the end somehow\n");

	return 0;

}
