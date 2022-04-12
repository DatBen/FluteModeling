//
//  utils.h
//

#ifndef utils_h
#define utils_h

#include <stdio.h>
#include <stdlib.h>

#define IO_BUF_LEN 8192
#define MAX_DIM 3

#define VTK_FILE_ERROR 101

void write_to_vtk (char *filename, float *rho, float *u, float *v, unsigned int nx, unsigned int ny);
void save(float *rho, float *u, float *v, int i, unsigned int size_x, unsigned int size_y);

#endif /* utils_h */
