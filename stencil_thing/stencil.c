/* stencil.c
 * Brian Gravelle
 * This code doesn't do anything useful
*/

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

#ifndef N
#define N 10   // the order of the matrix
#endif
#ifndef N
#define N 2   // the number of timesteps
#endif
#define HVAL   200.0    // initial value of High areas
#define LVAL   5.0      // initial value of Low areas
#define WIEGHT 0.5      // how much the next step is based on the previous vs the surrounding
                        // i.e. next = WIEGHT * prev + (1-WIEGHT) * halo


// global arrays cause I"m lazy
double A[N][N];
double B[N][N];

void matrix_init(void);
void step(void);

int main(int argc, char **argv) {
	int correct;
	int err = 0;
	double run_time;
	double mflops;


	double start, end;

	start = omp_get_wtime();

	//TODO MPI INIT

	matrix_init();

	//TODO the simulation

	end = omp_get_wtime();

	return 0;
}


void step(void) {

	int i = 0, j = 0, ii = 0, jj = 0;
	int hi = -1, hj = -1;
	double halo_sum = 0;

	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {

			halo_sum = 0;
			for (hi = -1; hi <= 1; hi++) {
				for ( hj = -1; hj <= 1; hj++) {
					ii = i+hi; jj = j+hj;
					if( (ii < 0) || (ii > N) || (jj < 0) || (jj > N) ) {
						halo_sum += A[i][j];
					} else if( (ii != 0) && (jj != 0) ) {
						halo_sum += A[ii][jj];
					}
				}
			}
			B[i][j] = halo_sum * 0.125;

		}
	}

	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {
			A[i][j] = WIEGHT * A[i][j] + (1-WIEGHT) * B[i][j];
		}
	}

}

// TODO this may need to change with the rank
void matrix_init(void) {
	int i, j;

	// A[N][P] -- Matrix A
	for (i = 0; i < 3; i++) {
		for (j = 0; j < N; j++) {
			A[i][j] = HVAL;
		}
	}
	for (i; i < N; i++) {
		for (j = 0; j < N; j++) {
			A[i][j] = LVAL;
		}
	}

	// B[P][M] -- Matrix B
	for (i = 0; i < N; i++) {
		for (j = 0; j < N; j++) {
			B[i][j] = 0;
		}
	}

}
