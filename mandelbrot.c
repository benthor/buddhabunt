#include <stdio.h>
#include "SDL/SDL.h"

#define WIDTH 640
#define HEIGHT 480
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


/*
void iterate_plane(int MaxX, int MaxY, float zoom_factor, float max_square_absolute, int curr_iteration, int max_iteration, float min_c_real, float min_c_img) {
	int pix_x,pix_y = 0;
	float c_real, c_img;

	for (pix_x=0; pix_x < MaxX; pix_x++) {
		c_real = (min_c_real + pix_x) * zoom_factor;
		for (pix_y=0; pix_y < MaxY; pix_y++) {
			c_img = (min_c_img + pix_y) * zoom_factor;
			printf("%i", iterate_point(c_real, c_img, max_square_absolute, curr_iteration));
		}
		printf("\n");
	}
}*/

void setPixel(SDL_Surface* screen, int x, int y, float shade) {
	//fprintf(stderr, "x:%i, y:%i\n", x,y);
	Uint32* pixmem32; //this is some _really weird_, compact representation apparently
	
	pixmem32 = (Uint32*) screen->pixels + x + y; //TODO: check how results differ when order of y and x is swapped

	// TODO: check out if we could do cool stuff with an alpha channel here as well
	//fprintf(stderr, "before\n");
	*pixmem32 = SDL_MapRGB(screen->format, (Uint8)(shade*255), (Uint8)(shade*255), (Uint8)(shade*255));
	//*pixmem32 = SDL_MapRGB(screen->format, 1, 1, 1);
	//fprintf(stderr, "after\n");
}



void iterate_plane(int iteration, SDL_Surface* screen) {
	float img_min = -2;
	float img_max = 2;
	float real_min = -3;
	float real_max = 2;


	int pix_x, pix_y;
	float c_real, c_img;
	int w = screen->w;
	int h = screen->h;
	int x,y;

	float x_zoom = (real_max-real_min)/w;
	float y_zoom = (img_max-img_min)/h;


	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}

	for (y=0; y<h; y++){
		for (x=0; x<w; x++) {
			c_img = img_min + x*x_zoom;
			c_real = real_min + y*y_zoom;
			if ( iteration == iterate_point(c_real, c_img, 5, iteration)) {
				setPixel(screen, x, y*w, 1);
			} else {
				setPixel(screen, x, y*w, 0.5);
			}
		}
		if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		SDL_Flip(screen);
	}

}



int main(int argc, char* argv[]) {

	SDL_Surface* screen;
	SDL_Event event;

	int keypress = 0;
	int iteration = 1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init video: %s\n", SDL_GetError());
		return 1;
	}

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE|SDL_HWSURFACE))) {
		fprintf(stderr, "Unable to set 640x480 video: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	
	while(1) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					fprintf(stderr, "quitting!\n");
					SDL_Quit();
					return 0;
					break;
				case SDL_KEYDOWN:
					iterate_plane(iteration++, screen);
					break;
			}
		}
	}

	fprintf(stderr, "have reached the end somehow\n");

	return 0;

}
