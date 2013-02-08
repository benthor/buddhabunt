#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include "SDL/SDL.h"
#include <math.h>

#define MINITER 9500
#define MAXITER 1600000
#define FACTOR 12
#define WIDTH (780 * FACTOR)
#define HEIGHT (1040 * FACTOR)
#define BUFFSIZE (WIDTH*HEIGHT)
#define	CURRSIZE (MAXITER*2)
#define BPP 4
#define DEPTH 32
#define IMG_MIN -1.125
#define IMG_MAX 1.125
#define REAL_MIN -2
#define REAL_MAX 1

#define X_ZOOM (((double)IMG_MAX-IMG_MIN)/WIDTH)
#define Y_ZOOM (((double)REAL_MAX-REAL_MIN)/HEIGHT)

#define NEXT_REAL(curr_real, curr_img, point_real) (curr_real*curr_real - curr_img*curr_img + point_real)
#define NEXT_IMAG(curr_real, curr_img, point_img) (2*curr_real*curr_img + point_img)

#define X2IMG(x) (IMG_MIN + x*X_ZOOM)
#define Y2REAL(y) (REAL_MIN + y*Y_ZOOM)

#define IMG2X(img) ((int)((img-IMG_MIN)/X_ZOOM)%WIDTH)
#define REAL2Y(real) ((int)((real-REAL_MIN)/Y_ZOOM)%HEIGHT)

#define SQR(x) ((x)*(x))


// c : point in the complex plane
// z : critical point (which for n iterations either stays within a radius s or escapes)
// s : the radius that z must not exceed after n iterations
// n : iterations after which a "still not diverged" z means that c can be considered to be in the set
int iterate_point(complex double c, double s, int n) {
	int iteration = 0;
	complex double z = 0 + 0*I;

	//cabs(z) function call is expensive... probably does a sqrt somewhere
	while ( cabs(z) <= s && iteration++ < n) {
		z = z*z + c;
	}

	return iteration;
}


// optimized implementation of the above 
// added array pointer to write values to
int opt_iterate_point(double c_real, double c_imag, double s, int n, complex double* path) {
	int iteration = 0;
	double z_real = 0;
	double z_imag = 0;
	double z_real_next, z_imag_next;
	double q = SQR(c_real - 0.25) + SQR(c_imag);
	//double q = SQR(c_real) - c_real/2 + 1/16 + SQR(c_imag);

	if (q * (q + (c_real - 0.25)) < 0.25 * SQR(c_imag)) { 
		return n;
		//return n;	
	}
	if (SQR(c_real + 1) + SQR(c_imag) < 0.0625) {
		//fprintf(stderr, "secondary cutoff!\n");	
		return n;
	}
	
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



void setPixel(SDL_Surface* screen, int x, int y, double shade, double ratio) {
	Uint32* pixmem32; 
	double r,g,b;	
	/*r = 2*(0.5 - ratio);
	if (r < 0) r = 0;
	g = ratio/0.5;
	if (g > 1) g = 2 - g;
	b = 2*(-0.5 + ratio);
	if (b < 0) b = 0;*/

	if (x < 0 || y < 0) return;

	r=1;
	g=1;
	b=1;
	
	pixmem32 = (Uint32*) screen->pixels + x + y*screen->w; 

	// TODO: check out if we could do cool stuff with an alpha channel here as well
	*pixmem32 = SDL_MapRGB(screen->format, (Uint8)(r*shade*255), (Uint8)(g*shade*255), (Uint8)(b*shade*255));
	//fprintf(stderr, "after\n");
}



void print_array(SDL_Surface* screen, int* a, int l, double ratio) {
	int i;
	int max=0;
	for (i=0; i<l; i++) {
		if (a[i] > max) {
			max = a[i];
		}
	}
	for (i=0; i<l; i++) {
		setPixel(screen, i%WIDTH, i/WIDTH, exp(log((double)a[i]/max)/3), ratio);
	}
}

void print_color_array(SDL_Surface* screen, double* r, double* g, double* b, int l) {
	Uint32* pixmem32;	
	int i;
	double maxr=1;
	double maxg=0;
	double maxb=0;
	for (i=0; i<l; i++) {
		if (r[i] > maxr) maxr = r[i];
		if (g[i] > maxg) maxg = g[i];
		if (b[i] > maxb) maxb = b[i];
	}
	//fprintf(stderr, "max calculated\n");
	//fprintf(stderr, "maxr %f, maxg %f, maxb %f\n", maxr, maxg, maxb);
	for (i=0; i<l; i++) {
		//fprintf(stderr, "pixmem\n");
		pixmem32 = (Uint32*) screen->pixels + i;
		//fprintf(stderr, "pixmem done\n");
		*pixmem32 = SDL_MapRGB(screen->format, (Uint8)(exp(log(r[i]/maxr)/3)*255), (Uint8)(exp(log(g[i]/maxg)/3)*255), (Uint8)(exp(log(b[i]/maxb)/3)*255));
		//*pixmem32 = SDL_MapRGB(screen->format, (Uint8)r[i]/maxr)*255, (Uint8)sqrt(g[i]/maxg)*255, (Uint8)sqrt(b[i]/maxb)*255);
		//setPixel(screen, i%WIDTH, i/WIDTH, exp(log((double)a[i]/max)/3), ratio);
	}
	//fprintf(stderr, "first printed\n");
}

void iterate_plane(int n, SDL_Surface* screen) {
	int x,y,i,iteration,offset;
	static complex double path[MAXITER];
	complex double z;
	static double r[WIDTH*HEIGHT];
	static double g[WIDTH*HEIGHT];
	static double b[WIDTH*HEIGHT];
	static double red,green,blue,ratio;

	int max_until_diverge = 0;
	
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0 ) return;
	}


	for (y=0; y<HEIGHT; y++){
		for (x=0; x<WIDTH; x++) {
			
			iteration = opt_iterate_point(Y2REAL(y), X2IMG(x), 2, n, &path);
			if (rand() % 5 != 0) continue;
			if (iteration > MINITER && iteration < MAXITER) {
				for (i=0; i<iteration; i++) {
					z = path[i];
					offset = IMG2X(cimag(z))+REAL2Y(creal(z))*WIDTH;
					if (offset >= 0) {
						//ratio = (double)(iteration-MINITER)/(MAXITER-MINITER);
						//ratio = (double)i*6.2832/MAXITER;
						ratio = (double)i*6.2832/iteration ;
						if (max_until_diverge < iteration) {
							max_until_diverge = iteration;
							fprintf(stderr, "max iteration found so far: %i\n", max_until_diverge);
						}
						if (ratio > M_PI) {
							r[offset] += (cos(ratio) + 1)*0.5;
						} else {
							b[offset] += (cos(ratio) + 1)*0.5;
						}
						//red = 2*(0.5 - ratio);
						//if (red < 0) red = 0;
						//r[offset] += red;
					
						g[offset] += (-cos(ratio) + 1) * 0.5;	
						//green = ratio/0.5;
						//if (green > 1) green = 2 - green;
						//g[offset] += green;
							
						//blue = 2*(-0.5 + ratio);
						//if (blue < 0) blue = 0;
						//b[offset] += blue;
					}
				}
			}
		}
		//HACK
		if (y%(10*FACTOR) != 0) continue;
		print_color_array(screen, &r, &g, &b, WIDTH*HEIGHT);
		if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
		SDL_Flip(screen);
		fprintf(stderr, "line nr %i of %i\n", y, HEIGHT);
	}
	//print_array(screen, &a, WIDTH*HEIGHT, 0.1);
	print_color_array(screen, &r, &g, &b, WIDTH*HEIGHT);
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}



int main(int argc, char* argv[]) {

	SDL_Surface* screen;
	SDL_Event event;

	int keypress = 0;
	int iteration = MAXITER;

	char filename[100];

	SDL_RWops* file;

	sprintf(filename, "output_%ix%i_from%ito%i.bmp", WIDTH, HEIGHT, MINITER, MAXITER);
	
	file = SDL_RWFromFile(filename, "wb");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init video: %s\n", SDL_GetError());
		return 1;
	}

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE|SDL_HWSURFACE))) {
		fprintf(stderr, "Unable to set %ix%i video: %s\n", WIDTH, HEIGHT, SDL_GetError());
		SDL_Quit();
		return 1;
	}

	iterate_plane(iteration, screen);
	SDL_SaveBMP_RW(screen, file, 1);
	SDL_Quit();
	return 1;
	
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
					fprintf(stderr, "quitting!\n");
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
