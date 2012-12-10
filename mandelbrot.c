#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include "SDL/SDL.h"

#define MINITER 10000
#define MAXITER 100000
#define SQR_ABS_MAX 7
#define WIDTH 1024
#define HEIGHT 768
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

// squared absolute of complex number
#define SQR_ABS(real,imag) (real*real + imag*imag)

#define RAND_X (rand()%WIDTH)
#define RAND_Y (rand()%HEIGHT)


// c : point in the complex plane
// z : critical point (which for n iterations either stays within a radius s or escapes)
// s : the radius that z must not exceed after n iterations
// n : iterations after which a "still not diverged" z means that c can be considered to be in the set
int iterate_point(complex float c, float s, int n) {
	int iteration = 0;
	complex float z = 0 + 0*I;
	//complex float next_z;
	//float real = 0;
	//float img = 0;


	//float real_tmp, img_tmp;

	while ( cabs(z) <= s && iteration++ < n) {
		z = z*z + c;
		//z = next_z;
		//real_tmp = NEXT_REAL(real,img,c_real);
		//img_tmp = NEXT_IMAG(real,img,c_img);
		//img = img_tmp; real = real_tmp;

				
		// slower:
		//current = creal(current)*creal(current) - cimag(current)*cimag(current) + creal(cpoint)
		//	  + (2*creal(current)*cimag(current) + cimag(cpoint))*I;
		// slowest:
		//current = iterate_step(current, cpoint);
		

	
		// fastest:
		//square_absolute = SQR_ABS(real,img);
		// slower:
		//square_absolute = creal(current)*creal(current) + cimag(current)*cimag(current);
		// slowest:
		//square_absolute = cabs(current); 
	}

	return iteration;
}


// optimized implementation of the above 
// added array pointer to write values to
int opt_iterate_point(float c_real, float c_imag, float s, int n, complex float* path) {
	int iteration = 0;
	float z_real = 0;
	float z_imag = 0;
	float z_real_next, z_imag_next;
	
	// optimization: replace sqrt in calc of abs on the on side with a square on the other:
	s = s*s;
	// so no need to use sqrt here:
	while (z_real*z_real + z_imag*z_imag <= s && iteration < n) {
		z_real_next = NEXT_REAL(z_real,z_imag,c_real);
		z_imag_next = NEXT_IMAG(z_real,z_imag,c_imag);
		z_real = z_real_next;
		z_imag = z_imag_next;
		path[iteration++] = z_real + z_imag*I;
	}
	return iteration;
}



void setPixel(SDL_Surface* screen, int x, int y, float shade) {
	if (x < 0 || y < 0) return;
	Uint32* pixmem32; 
	
	pixmem32 = (Uint32*) screen->pixels + x + y*screen->w; 

	// TODO: check out if we could do cool stuff with an alpha channel here as well
	*pixmem32 = SDL_MapRGB(screen->format, (Uint8)(shade*255), (Uint8)(shade*255), (Uint8)(shade*255));
	//fprintf(stderr, "after\n");
}



void print_array(SDL_Surface* screen, int* a, int l) {
	int i;
	int max=0;
	for (i=0; i<l; i++) {
		if (a[i] > max) {
			max = a[i];
			//fprintf(stderr, "max value: %i\n", max);
		}
	}
	for (i=0; i<l; i++) {
		setPixel(screen, i%WIDTH, i/WIDTH, (float)a[i]/max);
	}
}




void iterate_plane(int n, SDL_Surface* screen) {
	int x,y,i,iteration,offset;
	static complex float path[MAXITER];
	complex float z;
	static int a[WIDTH*HEIGHT];	
	
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}


	for (y=0; y<HEIGHT; y++){
		for (x=0; x<WIDTH; x++) {
			//setPixel(screen, x, y, (float)(iterate_point(Y2REAL(y) + X2IMG(x)*I, 2, iteration))/iteration);
			//setPixel(screen, x, y, (float)(opt_iterate_point(Y2REAL(y), X2IMG(x), 2, iteration, path))/iteration);
			iteration = opt_iterate_point(Y2REAL(y), X2IMG(x), 2, n, &path);
			if (iteration < MAXITER && iteration > MINITER) {
				for (i=0; i<iteration; i++) {
					z = path[i];
					//setPixel(screen, IMG2X(cimag(z)), REAL2Y(creal(z)), (float)iteration/MAXITER);
					offset = IMG2X(cimag(z))+REAL2Y(creal(z))*WIDTH;
					if (offset >= 0 && offset < WIDTH*HEIGHT) {
						a[offset]++;
					}
				}
			}
		}
		print_array(screen, &a, WIDTH*HEIGHT);
		if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		SDL_Flip(screen);

	}
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}



int main(int argc, char* argv[]) {

	SDL_Surface* screen;
	SDL_Event event;

	int keypress = 0;
	int iteration = MAXITER;

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
	//random_buddha_plane(screen, 2000, 10000, WIDTH*HEIGHT/100);	
	//SDL_SaveBMP_RW(screen, file, 1);
	//SDL_Quit();
	//return 0;
	
	while(1) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					fprintf(stderr, "quitting!\n");
					SDL_SaveBMP_RW(screen, file, 1);
					return 0;
					break;
				case SDL_KEYDOWN:
					//iterate_plane(iteration++, screen);
					SDL_SaveBMP_RW(screen, file, 1);
					SDL_Quit();
					return 0;
					break;
				case SDL_MOUSEBUTTONDOWN:
					//iterate_plane(iteration++, screen);
					fprintf(stderr, "mouse x:%i, mouse y:%i\n", event.button.x, event.button.y);
					//trace_point(screen, Y2REAL(event.button.y), X2IMG(event.button.x), 0, 10);
					break;
			}
		}
	}

	fprintf(stderr, "have reached the end somehow\n");

	return 0;

}
