#include <stdio.h>
#include "SDL/SDL.h"

#define WIDTH 1024
#define HEIGHT 768
#define BPP 4
#define DEPTH 32

int iterate_point(float c_real, float c_img, float max_square_absolute, int max_iteration) {
	float square_absolute = 0;
	int iteration = 0;
	float real = 0;
	float img = 0;


	float real_tmp, img_tmp;

	while ( square_absolute <= max_square_absolute && iteration < max_iteration ) {
		real_tmp = real*real - img*img + c_real;
		img_tmp = 2*real*img + c_img;
		img = img_tmp; real = real_tmp;
		iteration++;
		square_absolute = real*real + img*img;
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



void iterate_plane(int iteration, SDL_Surface* screen) {
	float img_min = -2;
	float img_max = 2;
	float real_min = -2;
	float real_max = 1;


	int pix_x, pix_y;
	float c_real, c_img;
	int w = screen->w;
	int h = screen->h;
	int x,y;

	float y_zoom = (real_max-real_min)/h;
	float x_zoom = (img_max-img_min)/w;


	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}

	for (y=0; y<h; y++){
		for (x=0; x<w; x++) {
			c_img = img_min + x*x_zoom;
			c_real = real_min + y*y_zoom;
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
	int iteration = 0;

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
