#include "../include/utils.h"

int build_context(cl_platform_id *pplatforms, cl_device_id *pdevice,
                  cl_context *pcontext, cl_command_queue *pqueue)
{
    cl_int err;
    err = clGetPlatformIDs(1, pplatforms, NULL);
    if (err)
        return err;
    err = clGetDeviceIDs(*pplatforms, CL_DEVICE_TYPE_GPU, 1, pdevice, NULL);
    if (err)
        return err;
    *pcontext = clCreateContext(NULL, 1, pdevice, NULL, NULL, &err);
    if (err)
        return err;
    *pqueue = clCreateCommandQueueWithProperties(*pcontext, *pdevice, 0, &err);
    return err;
}

int build_kernels(char *filename, int nb_kernels, cl_kernel *kernels,
                  char **kernel_names, cl_context context,
                  cl_device_id *pdevice)
{
    cl_int err;
    // lecture du fichier dans un buffer
    FILE *cl_file;
    cl_file = fopen("flute.cl", "r");
    // fopen_s( & cl_file, "flute.cl", "r");
    if (!cl_file)
    {
        return 1;
    }
    char *cl_buffer = (char *)malloc(10000);
    size_t nb_char = fread(cl_buffer, 1, 10000, cl_file);
    // construction du prg
    cl_program prg;
    prg = clCreateProgramWithSource(context, 1, (const char **)&cl_buffer,
                                    &nb_char, &err);
    if (err)
        exit(101);
    // compilation
    const char *options = "-cl-opt-disable";
    err = clBuildProgram(prg, 1, pdevice, options, NULL, NULL);
    if (err)
    {
        printf("%d\n", err);
        exit(102);
    }
    free(cl_buffer);
    for (int i = 0; i < nb_kernels; ++i)
    {
        kernels[i] = clCreateKernel(prg, kernel_names[i], &err);
        // printf(">>%p\n", kernels[i]);
        if (err)
            return err;
    }
    return err;
}
void write_to_vtk(char *filename, float *rho, float *u, float *v,
                  unsigned int nx, unsigned int ny)
{
    int vtkscalarlength;
    int vtkoffset;
    float realbuffer[IO_BUF_LEN + MAX_DIM];
    int intbuffer;
    FILE *fid;

    unsigned int ibuf, i, j;

    fid = fopen(filename, "w");
    if (!fid)
        exit(VTK_FILE_ERROR);

    vtkscalarlength = nx * ny * sizeof(*realbuffer);
    vtkoffset = 0;
    fprintf(fid,
            "<?xml version=\"1.0\"?>\n<VTKFile type=\"ImageData\" "
            "version=\"0.1\" byte_order=\"LittleEndian\">\n"
            "  <ImageData WholeExtent=\"%d %d %d %d %d %d\" GhostLevel=\"1\" "
            "Origin=\"0. 0. 0.\" Spacing=\"1 1 1\">\n"
            "    <Piece Extent=\"%d %d %d %d %d %d\">\n"
            "      <PointData>\n",
            1, nx, 1, ny, 1, 1, 1, nx, 1, ny, 1, 1);

    fprintf(fid,
            "        <DataArray type=\"Float32\" Name=\"density\"  "
            "NumberOfComponents=\"1\" format=\"appended\" offset=\"%d\"/>\n",
            vtkoffset);
    vtkoffset = vtkoffset + (1 * vtkscalarlength + sizeof(intbuffer));
    fprintf(fid,
            "        <DataArray type=\"Float32\" Name=\"velocity\" "
            "NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n",
            vtkoffset);
    vtkoffset = vtkoffset + (3 * vtkscalarlength + sizeof(intbuffer));

    fprintf(fid,
            "      </PointData>\n"
            "      <CellData>\n"
            "      </CellData>\n"
            "    </Piece>\n"
            "  </ImageData>\n"
            "  <AppendedData encoding=\"raw\">\n\n_");
    ibuf = 0;

    // Output density
    intbuffer = 1 * vtkscalarlength;
    fwrite(&intbuffer, sizeof(intbuffer), 1, fid);
    for (j = 0; j < ny; j++)
    {
        for (i = 0; i < nx; i++)
        {
            realbuffer[ibuf] = (rho[i + nx * j]);
            ibuf = ibuf + 1;
            if (ibuf >= IO_BUF_LEN)
            {
                fwrite(realbuffer, sizeof(*realbuffer), ibuf, fid);
                ibuf = 0;
            }
        }
    }

    if (ibuf > 0)
    {
        fwrite(realbuffer, sizeof(*realbuffer), ibuf, fid);
        ibuf = 0;
    }

    // Output velocity
    intbuffer = 3 * vtkscalarlength;
    fwrite(&intbuffer, sizeof(intbuffer), 1, fid);
    for (j = 0; j < ny; j++)
    {
        for (i = 0; i < nx; i++)
        {
            realbuffer[ibuf] = u[i + nx * j];
            ibuf = ibuf + 1;
            realbuffer[ibuf] = v[i + nx * j];
            ibuf = ibuf + 1;
            realbuffer[ibuf] = 0.0f;
            ibuf = ibuf + 1;
            if (ibuf >= IO_BUF_LEN)
            {
                fwrite(realbuffer, sizeof(*realbuffer), ibuf, fid);
                ibuf = 0;
            }
        }
    }

    if (ibuf > 0)
    {
        fwrite(realbuffer, sizeof(*realbuffer), ibuf, fid);
        ibuf = 0;
    }

    fprintf(fid,
            "\n  </AppendedData>\n"
            "</VTKFile>\n");
    fclose(fid);
}

void save(float *rho, float *u, float *v, int i, unsigned int size_x,
          unsigned int size_y)
{
    char *filename;
    int filename_length = 24;
    filename = (char *)malloc(filename_length * sizeof(char));
    sprintf(filename, "res%08d.vti", i);
    write_to_vtk(filename, rho, u, v, size_x, size_y);
    printf("%s\n", filename);
    free(filename);
}
