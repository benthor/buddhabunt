#include <stdio.h>
#include <stdlib.h>
//#include <complex.h>
#include "SDL/SDL.h"
#include <time.h>
#include <math.h>
#include <sys/select.h>
#include <unistd.h>

#define MINITER 1000
#define MAXITER 10000000
#define FACTOR 2
#define WIDTH (390 * FACTOR)
#define HEIGHT (520 * FACTOR)
#define BUFFSIZE (WIDTH*HEIGHT)
#define	CURRSIZE (MAXITER*2)

#define LOOP_MAX_DIAMETER 1000
#define LOOP_CHECK_EVERY 10000


#define MAX_ABS 2

#define BPP 4
#define DEPTH 32
#define IMG_MIN -1.125
#define IMG_MAX 1.125
#define REAL_MIN -2
#define REAL_MAX 1

#define WORKERS 2

#define X_ZOOM (((double)IMG_MAX-IMG_MIN)/WIDTH)
#define Y_ZOOM (((double)REAL_MAX-REAL_MIN)/HEIGHT)

#define NEXT_REAL(curr_real, curr_img, point_real) (curr_real*curr_real - curr_img*curr_img + point_real)
#define NEXT_IMAG(curr_real, curr_img, point_img) (2*curr_real*curr_img + point_img)

#define X2IMG(x) (IMG_MIN + x*X_ZOOM)
#define Y2REAL(y) (REAL_MIN + y*Y_ZOOM)

#define IMG2X(img) ((int)((img-IMG_MIN)/X_ZOOM)%WIDTH)
#define REAL2Y(real) ((int)((real-REAL_MIN)/Y_ZOOM)%HEIGHT)

#define SQR(x) ((x)*(x))

/*
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
}*/


int cutoffs = 0;
int orbits = 0;
int loops = 0;



static int loopdetector(double* path, int length) {
  double z_real = path[length-2];
  double z_imag = path[length-1];
  
  // only take the last 10 percent of the orbit length into account
  for (int i = length-4; i > 0 && i > (length-LOOP_MAX_DIAMETER); i-=2) {
    if (path[i] == z_real) {
      if (path[i+1] == z_imag){
	//fprintf(stderr, "loop found after %i iterations, diameter: %i\n", length, length-i);
	loops++;
	return 1;
      } else {
	//fprintf(stderr, "found duplicate real coordinate, but imag not matching\n");
	return 0;
      }
    }
  }
  return 0;
}


// optimized implementation of the above 
// added array pointer to write values to
static int opt_iterate_point(double c_real, double c_imag) {
  int iteration = 0;
  double z_real = 0;
  double z_imag = 0;
  double z_real_next, z_imag_next;
  double q = SQR(c_real - 0.25) + SQR(c_imag);
  static double path[2*MAXITER];


  // filter out main mandelbrot cardioid
  if (q * (q + (c_real - 0.25)) < 0.25 * SQR(c_imag)) { 
    return MAXITER; 
   //return n;	
  }
  if (SQR(c_real + 1) + SQR(c_imag) < 0.0625) {
    //fprintf(stderr, "secondary cutoff!\n");	
    return MAXITER;
  }
	
  // optimization: replace sqrt in calc of abs on the on side with a square on the other:
  int s = SQR(MAX_ABS);
  // so no need to use sqrt here:
  while (z_real*z_real + z_imag*z_imag <= s) {
    if (iteration == MAXITER || (iteration % LOOP_CHECK_EVERY == 0 && loopdetector(path, iteration*2))) {
       cutoffs++;
       return MAXITER;
     }

     z_real_next = NEXT_REAL(z_real,z_imag,c_real);
     z_imag_next = NEXT_IMAG(z_real,z_imag,c_imag);
     z_real = z_real_next;
     z_imag = z_imag_next;


     path[2*iteration] = z_real;
     path[2*iteration+1] = z_imag;
     iteration++;

   }

   orbits++;
   return iteration;
 }



 /*static void setPixel(SDL_Surface* screen, int x, int y, double shade, double ratio) {
   Uint32* pixmem32; 
   double r,g,b;	

   if (x < 0 || y < 0) return;

   r=1;
   g=1;
   b=1;

   pixmem32 = (Uint32*) screen->pixels + x + y*screen->w; 

   // TODO: check out if we could do cool stuff with an alpha channel here as well
   *pixmem32 = SDL_MapRGB(screen->format, (Uint8)(r*shade*255), (Uint8)(g*shade*255), (Uint8)(b*shade*255));
 }



 static void print_array(SDL_Surface* screen, int* a, int l, double ratio) {
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
   }*/

 static void print_color_array(SDL_Surface* screen, double* r, double* g, double* b, int l) {
   Uint32* pixmem32;	
   int i;
   double maxr=0;
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
     pixmem32 = (Uint32*) screen->pixels + i;
     //squeeze dynamic range by taking the third root of the ratios (which are doubles between 0 and 1)
     *pixmem32 = SDL_MapRGB(screen->format, (Uint8)(exp(log(r[i]/maxr)/3)*255), (Uint8)(exp(log(g[i]/maxg)/3)*255), (Uint8)(exp(log(b[i]/maxb)/3)*255));
   }
 }

void add_known_orbit(double* r, double* g, double* b, double c_real, double c_imag, int iteration) {
   double z_real = c_real;
   double z_imag = c_imag;

   for (int i=0; i<iteration; i++) {
     int offset = IMG2X(z_imag)+REAL2Y(z_real)*WIDTH;
     // maybe useless check
     if (offset >= 0) {
       double ratio = (double)i*6.2832/iteration;
       if (ratio > M_PI) {
	 r[offset] += ( SQR(cos(ratio)) + 1) * 0.5;
       } else {
	 b[offset] += ( SQR(cos(ratio)) + 1) * 0.5;
       }
       g[offset]   += (-SQR(cos(ratio)) + 1) * 0.5;	
     }

     double z_real_next = NEXT_REAL(z_real, z_imag, c_real);
     double z_imag_next = NEXT_IMAG(z_real, z_imag, c_imag);

     z_real = z_real_next;
     z_imag = z_imag_next;
   }
 }

/* static void iterate_plane(SDL_Surface* screen) {
   static double path[MAXITER*2];

   if (SDL_MUSTLOCK(screen)) {
     if (SDL_LockSurface(screen) < 0 ) return;
   }


   for (int y=0; y<HEIGHT; y++){
     for (int x=0; x<WIDTH; x++) {
       if (rand() % 5 != 0) continue;
       int iteration = opt_iterate_point(Y2REAL(y), X2IMG(x));
       if (iteration > MINITER && iteration < MAXITER) {
	 add_known_orbit(screen, Y2REAL(y), X2IMG(x), iteration);
       }
     }
     //HACK
     if (y%(10*FACTOR) == 0) {
       if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
       SDL_Flip(screen);
       fprintf(stderr, "line nr %i of %i\n", y, HEIGHT);
     }
   }
   if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
   SDL_Flip(screen);
   }*/

int is_readable(int fd) {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = tv.tv_usec = 0;

  FD_ZERO(&fds);
  FD_SET(fd,&fds);

  int retval = select(fd+1, &fds, NULL, NULL, &tv);

  if (retval == -1) {
    return 0;
    //perror("select()");
  }

  return FD_ISSET(fd,&fds) ? 1 : 0;
}

int is_writable(int fd) {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = tv.tv_usec = 0;

  FD_ZERO(&fds);
  FD_SET(fd,&fds);

  int retval = select(fd+1, NULL, &fds, NULL, &tv);

  if (retval == -1) {
    return 0;
    //perror("select()");
  }

  return FD_ISSET(fd,&fds) ? 1 : 0;
}


void iterate_plane_fds(SDL_Surface* screen, int* rfds) {
  static double r[WIDTH*HEIGHT];
  static double g[WIDTH*HEIGHT];
  static double b[WIDTH*HEIGHT];
  double real, imag;
  int iteration;
  int tmp = WORKERS;
  int i = 0;

  while (tmp) {
    for (int i=0; i<WORKERS; i++) {
      int rfd = rfds[i];
      if (is_readable(rfd)) {
	read(rfd, &iteration, sizeof(int)); 
	if (!iteration) {
	  tmp--;
	  close(rfd);
	  continue;
	}
	if (read(rfd, &real, sizeof(double)) == -1) fprintf(stderr, "whoops\n");
	if (read(rfd, &imag, sizeof(double)) == -1) fprintf(stderr, "whoops\n");
	//fprintf(stderr, "got point to draw at %f : %f with iteration depth of %i!\n", real, imag, iteration);
	add_known_orbit(&r, &g, &b, real, imag, iteration);
	if (i++ % 50*FACTOR == 0) {
	  print_color_array(screen, &r, &g, &b, WIDTH*HEIGHT);
	  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	  SDL_Flip(screen);

	}
      }
    }
  }
  print_color_array(screen, &r, &g, &b, WIDTH*HEIGHT);
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  SDL_Flip(screen);
}

void worker(int wfd, int workerid) {
  double real, imag;
  int iteration;
  int i;
  for (int y=0; y<(HEIGHT/WORKERS)*(workerid+1); y++) {
    for (int x=0; x<WIDTH; x++) {
      //if (rand() % 5*WORKERS != 0) continue;
      if (i++ % 5*WORKERS != 0) continue;
      real = Y2REAL(y);
      imag = X2IMG(x);
      //fprintf(stderr, "worker: got point to test %f : %f\n", real, imag);
      iteration = opt_iterate_point(real, imag);
      if (iteration > MINITER && iteration < MAXITER) {
	write(wfd, &iteration, sizeof(int));
	write(wfd, &real, sizeof(double));
	write(wfd, &imag, sizeof(double));
	fprintf(stderr, "worker %i: got point to draw at %f : %f with iteration depth of %i!\n", workerid, real, imag, iteration);
      }
    }
  }
  iteration = 0;
  write(wfd, &iteration, sizeof(int));
  close(wfd);
  fprintf(stderr, "worker terminating\n");
}

static int save(SDL_Surface* screen) {
  char filename[127];
  SDL_RWops* file;
  char timestamp[127];
  time_t t = time(NULL);

  if (!(strftime(timestamp, 127, "%F-%T", localtime(&t)))) {
    fprintf(stderr, "strftime returned 0\n");
  }
  sprintf(filename, "%s_output_%ix%i_from%ito%i.bmp", timestamp, WIDTH, HEIGHT, MINITER, MAXITER);
  if (!(file = SDL_RWFromFile(filename, "wb"))) { 
    fprintf(stderr, "Unable to open filename '%s', error: %s!\n", filename, SDL_GetError());
    return 1;
  }

  if (SDL_SaveBMP_RW(screen, file, 1)) {
    fprintf(stderr, "Unable to save image under filename '%s', error: %s!\n", filename, SDL_GetError());
    return 1;
  }
  fprintf(stderr, "surface dumped to file '%s'\n", filename);

  return 0;
}



int main(int argc, char* argv[]) {

  SDL_Surface* screen;
  SDL_Event event;

  int keypress = 0;

  // multiprocessing setup
  pid_t pids[WORKERS];
  int fds[WORKERS][2];
  
  
  for (int i=0; i<WORKERS; i++) {
    if (pipe(fds[i]) < 0) {
      fprintf(stderr, "pipe error\n");
      exit(1);
    }

    if ((pids[i] = fork()) < 0) {
      fprintf(stderr, "fork error\n");
      exit(1);
    }
  
    if (pids[i] == 0) {
      fprintf(stderr, "worker %i\n", i);
      srandom(i+2);
      close(fds[i][0]);
      worker(fds[i][1], i);
      return 0;
    }
  }
    
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Unable to init video: %s\n", SDL_GetError());
    return 1;
  }
      
  if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE|SDL_HWSURFACE))) {
    fprintf(stderr, "Unable to set %ix%i video: %s\n", WIDTH, HEIGHT, SDL_GetError());
    SDL_Quit();
    return 1;
  }

  int rfds[WORKERS];
  for (int i=0; i<WORKERS; i++) {
    rfds[i] = fds[i][0];
    close(fds[i][1]);
  }
  iterate_plane_fds(screen, &rfds);
    
    //iterate_plane(screen);
    
  //SDL_Quit();
  //  return 0;
  //fprintf(stderr, "render done:\n - legal orbits: %i\n - threshold overruns: %i\n - detected loops: %i\n", orbits, cutoffs, loops);

  while(1) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
	fprintf(stderr, "quitting!\n");
	return 0;
	
      case SDL_KEYDOWN:
	//iterate_plane(iteration++, screen);
	if (event.key.keysym.sym == SDLK_s) {
	  fprintf(stderr, "saving!\n");
	  save(screen); 
	} else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
	  fprintf(stderr, "quitting!\n");
	  SDL_Quit();
	  return 0;
	}
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
