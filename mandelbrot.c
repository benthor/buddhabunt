#include <stdio.h>

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
}

void iterate_plane2(float stepsize, int iteration) {
	int pix_x, pix_y;
	float c_real, c_img;
		
	for (c_img=-2; c_img < 2; c_img+=stepsize) {
		for (c_real=-3; c_real < 2; c_real+=stepsize) {
			if ( iteration == iterate_point(c_real, c_img, 5, iteration)) {
				printf("*");
			} else {
				printf(" ");
			}
		}
		printf("\n");
	}
}



int main(int argc, char* argv[]) {
	iterate_plane2(0.02,50);
}
