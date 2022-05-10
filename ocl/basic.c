#include <stdio.h>
#define CL_TARGET_OPENCL_VERSION 210
#include <CL/cl.h>
int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program prg;

    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);

    FILE *file;
    file = fopen("flute.cl", "r");

    char *buffer = (char *)malloc(10000);

    size_t lus = fread(buffer, 1, 10000, file);
    fclose(file);
    prg =


    printf("%d %d\n", err, lus);

    printf("Hello, World!\n");

    return 0;
}