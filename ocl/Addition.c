
#include <stdio.h>
#include <stdlib.h>
#define CL_TARGET_OPENCL_VERSION 210  // for linux
#include <CL/cl.h>

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program prg;
    cl_kernel kernel;
    err = clGetPlatformIDs(1, &platform, NULL);
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    FILE *file;
    // fopen_s(& file, "flute.cl", "r");
    file = fopen("flute.cl", "r");  // for linux
    char *buffer = (char *)malloc(10000);
    size_t lus;
    if (buffer && file) {
        lus = fread(buffer, 1, 10000, file);
        fclose(file);
        prg = clCreateProgramWithSource(context, 1, (const char **)&buffer,
                                        &lus, &err);
        err = clBuildProgram(prg, 1, &device, NULL, NULL, NULL);
        printf("%d\n", err);
        kernel = clCreateKernel(prg, "add", &err);
        printf("%d\n", err);
        cl_float alpha[100];
        cl_float beta[100];
        cl_float parametre = 0.3f;
        for (int i = 0; i < 100; ++i) {
            alpha[i] = i;
            beta[i] = 2 * i;
        }
        cl_mem mem_alpha = clCreateBuffer(
            context,
            CL_MEM_COPY_HOST_PTR |
                CL_MEM_READ_WRITE,  // droit de modifer sur la GPU
            sizeof(cl_float) * 100, alpha, &err);
        printf("alpha = %d\n", err);
        cl_mem mem_beta = clCreateBuffer(
            context,
            CL_MEM_COPY_HOST_PTR |
                CL_MEM_READ_WRITE,  // droit de modifer sur la GPU
            sizeof(cl_float) * 100, beta, &err);
        printf("beta = %d\n", err);
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &mem_alpha);
        err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &mem_beta);
        err = clSetKernelArg(kernel, 2, sizeof(cl_float), &parametre);
        printf("err = %d\n", err);
        cl_command_queue queue;
        queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);
        clEnqueueWriteBuffer(queue, mem_alpha, CL_TRUE, 0,
                             100 * sizeof(cl_float), alpha, 0, NULL, NULL);
        clEnqueueWriteBuffer(queue, mem_beta, CL_TRUE, 0,
                             100 * sizeof(cl_float), beta, 0, NULL, NULL);
        size_t global[] = {100};
        size_t local[] = {1};
        clEnqueueNDRangeKernel(queue, kernel, 1, 0, global, local, 0, NULL,
                               NULL);
        clEnqueueReadBuffer(queue, mem_alpha, CL_TRUE, 0,
                            100 * sizeof(cl_float), alpha, 0, NULL, NULL);
        for (int i = 0; i < 100; ++i) {
            printf("%f ", alpha[i]);
        }
    }
    return 0;
}
