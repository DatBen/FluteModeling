#include <stdio.h>
#include <stdlib.h>

#include "../include/flute.h"
#include "../include/utils.h"

// Définition du domaine de simulation

#define NUMEL (SIZE_X * SIZE_Y)

// Définition du réseau

#define LATTICE_D 2
#define LATTICE_Q 9
#define NB_KERNEL 6

int IDX(int q, int index, int offset_N[LATTICE_Q]) {
    // return q * NUMEL + i;				// Sans propagation
    return q * NUMEL + (index + offset_N[q]) %
                           NUMEL;  // Avec propagation grace au tableau offset_N
}

int lattice_c[LATTICE_Q][LATTICE_D] = {{0, 0},  {1, 0},   {0, 1},
                                       {-1, 0}, {0, -1},  {1, 1},
                                       {-1, 1}, {-1, -1}, {1, -1}};

cl_float lattice_w[LATTICE_Q] = {4.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,
                                 1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 36.0f,
                                 1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f};

int lattice_bounceback[LATTICE_Q] = {0, 3, 4, 1, 2, 7, 8, 5, 6};

cl_float lattice_inv_cs2 = 3.0f;

cl_kernel kernels[NB_KERNEL];
cl_command_queue pqueue;

// Collision

void calc_flow_properties_from_boltzmann_local(cl_float N[LATTICE_Q * NUMEL],
                                               cl_float *rho, cl_float *u,
                                               cl_float *v, int index,
                                               int offset_N[LATTICE_Q]) {
    // Une fonction qui calcule les propriétés locales de l'écoulement à partir
    // des distributions de Boltzmann
    *rho = 0.0f;
    *u = 0.0f;
    *v = 0.0f;
    for (int q = 0; q < LATTICE_Q; ++q) {
        *rho += N[IDX(q, index, offset_N)];
        *u += N[IDX(q, index, offset_N)] * lattice_c[q][0];
        *v += N[IDX(q, index, offset_N)] * lattice_c[q][1];
    }
    *u /= *rho;
    *v /= *rho;
}

void calc_flow_properties_from_boltzmann(cl_float N[LATTICE_Q * NUMEL],
                                         cl_float rho[NUMEL], cl_float u[NUMEL],
                                         cl_float v[NUMEL],
                                         int offset_N[LATTICE_Q]) {
    for (int i = 0; i < NUMEL; ++i) {
        calc_flow_properties_from_boltzmann_local(N, rho + i, u + i, v + i, i,
                                                  offset_N);
    }
}

void calc_equilibrium_from_flow_properties_local(cl_float rho, cl_float u,
                                                 cl_float v,
                                                 cl_float Neq[LATTICE_Q]) {
    // Une fontion qui calcule la distribution à l'équilibre correspondante aux
    // propriétés d'écoulement

    cl_float v2 = u * u + v * v;
    for (int q = 0; q < LATTICE_Q; ++q) {
        cl_float vc = u * lattice_c[q][0] + v * lattice_c[q][1];
        Neq[q] = rho * lattice_w[q] *
                 (1.0f + lattice_inv_cs2 *
                             (vc + 0.5f * (vc * vc * lattice_inv_cs2 - v2)));
    }
}

void collide_local(cl_float N[LATTICE_Q * NUMEL], cl_float tau, int index,
                   int offset_N[LATTICE_Q]) {
    cl_float Neq[LATTICE_Q];
    cl_float rho, u, v;

    calc_flow_properties_from_boltzmann_local(N, &rho, &u, &v, index, offset_N);

    calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);

    for (int q = 0; q < LATTICE_Q; ++q) {
        N[IDX(q, index, offset_N)] -=
            (N[IDX(q, index, offset_N)] - Neq[q]) / tau;
    }
}

void collide(cl_float N[LATTICE_Q * NUMEL], cl_float tau,
             int offset_N[LATTICE_Q]) {
    for (int index = 0; index < NUMEL; ++index) {
        collide_local(N, tau, index, offset_N);
    }
}

void ccollide() {
    size_t global[] = {NUMEL};
    size_t local[] = {1};
    cl_int err = clEnqueueNDRangeKernel(pqueue, kernels[1], 1, NULL, global,
                                        local, 0, NULL, NULL);
    if (err) exit(38);
}

// Initialisation

void init(cl_float N[LATTICE_Q * NUMEL], cl_float rho, cl_float u, cl_float v,
          int offset_N[LATTICE_Q]) {
    cl_float Neq[LATTICE_Q];
    calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);

    for (int q = 0; q < LATTICE_Q; ++q) {
        offset_N[q] = 0;
    }

    for (int index = 0; index < NUMEL; ++index) {
        for (int q = 0; q < LATTICE_Q; ++q) {
            N[IDX(q, index, offset_N)] = Neq[q];
        }
    }
}

void cinit(cl_mem cloffset_N, cl_int *offset_N) {
    for (int q = 0; q < LATTICE_Q; ++q) {
        offset_N[q] = 0;
    }
    cl_int err = clEnqueueWriteBuffer(pqueue, cloffset_N, CL_TRUE, 0,
                                      LATTICE_Q * sizeof(cl_int), offset_N, 0,
                                      NULL, NULL);
    if (err) exit(36);
    size_t global[] = {NUMEL};
    size_t local[] = {1};
    err = clEnqueueNDRangeKernel(pqueue, kernels[0], 1, NULL, global, local, 0,
                                 NULL, NULL);
    if (err) exit(37);
}

// Propagation

void stream(int offset_N[LATTICE_Q]) {
    for (int q = 1; q < LATTICE_Q; ++q) {
        /**** Attention ****/
        // offset_N[q] = (offset_N[q] + lattice_c[q][0] + lattice_c[q][1] *
        // SIZE_X) % NUMEL;
        //  Deux erreurs dans la ligne ci-dessus:
        //  1. le signe: décaler toutes les valeurs dans un sens (positif par
        //  exemple) revent à décaler le zéro (offset) dans l'autre sens
        //  (négatif).
        //  2. % n'est pas l'opérateur modulo et se comporte différemment pour
        //  les valeurs négatives, or que l'on ajoute ou soustraie l'offset, c
        //  peut avoir des composantes négatives ou positives, il faut donc
        //  gérer correctement les cas négatifs. Les 5 lignes ci-dessous sont un
        //  peu plus longues mais sont justes :
        offset_N[q] =
            (offset_N[q] - lattice_c[q][0] - lattice_c[q][1] * SIZE_X);
        if (offset_N[q] >= NUMEL) offset_N[q] -= NUMEL;
        if (offset_N[q] < 0) offset_N[q] += NUMEL;
    }
}

void cstream() {
    size_t global[] = {NUMEL};
    size_t local[] = {1};
    cl_int err = clEnqueueNDRangeKernel(pqueue, kernels[2], 1, NULL, global,
                                        local, 0, NULL, NULL);
    if (err) exit(38);
}

// Conditions limites

void walls(int wall[], int nb_wall, cl_float N[LATTICE_Q * NUMEL],
           int offset_N[LATTICE_Q]) {
    /* Parois */
    for (int i = 0; i < nb_wall; ++i) {
        int index = wall[i];

        for (int q = 0; q < LATTICE_Q; ++q) {
            int q_bb = lattice_bounceback[q];
            int index_orig =
                (index - (lattice_c[q][0] + lattice_c[q][1] * SIZE_X)) % NUMEL;

            cl_float temp = N[IDX(q_bb, index_orig, offset_N)];
            N[IDX(q_bb, index_orig, offset_N)] = N[IDX(q, index, offset_N)];
            N[IDX(q, index, offset_N)] = temp;
        }
    }
}
void cwalls() {
    size_t global[] = {NUMEL};
    size_t local[] = {1};
    cl_int err = clEnqueueNDRangeKernel(pqueue, kernels[3], 1, NULL, global,
                                        local, 0, NULL, NULL);
    if (err) exit(38);
}

void blows(int blow[], int nb_blow, cl_float N[LATTICE_Q * NUMEL],
           int offset_N[LATTICE_Q], cl_float p) {
    int dir[3] = {1, 5, 8};
    int dir_fs[3] = {3, 6, 7};  // free slip: glissement

    for (int i = 0; i < nb_blow; ++i) {
        int index = blow[i];

        for (int q = 0; q < 3; ++q) {
            N[IDX(dir[q], index, offset_N)] =
                N[IDX(dir_fs[q], index, offset_N)];
        }

        cl_float rho = 0.0f;
        for (int q = 0; q < LATTICE_Q; ++q) {
            rho += N[IDX(q, index, offset_N)];
        }

        for (int q = 0; q < 3; ++q) {
            N[IDX(dir[q], index, offset_N)] += (p - rho) * lattice_w[q];
        }
    }
}

void cblows(cl_float p) {
    size_t global[] = {NUMEL};
    size_t local[] = {1};
    cl_int err = clSetKernelArg(kernels[4], 2, sizeof(cl_float), &p);
    err = clEnqueueNDRangeKernel(pqueue, kernels[4], 1, NULL, global, local, 0,
                                 NULL, NULL);
    if (err) {
        printf("%d\n", err);
        exit(39);
    }
}

void border_bc(cl_float N[LATTICE_Q * NUMEL], int index, int dir[],
               int dir_fs[], int n_dir, int offset_N[LATTICE_Q]) {
    // dir: directions entrantes. Pour la condition limite considérées,
    // indices des vecteurs tels que c_i . n_bc > 0 (avec n_bc, vecteur
    // normal pointant vers l'intérieur du domaine). Par exemple pour une
    // condtion limite en x = 0, la normale entrante est e_x, donc dir = {1,
    // 5, 8}. dir_fs: direction de rebond des directions entrantes (ici
    // rebond seulement dans la direction normale pour laisser le fluide
    // glisser dans la direction tangentielle). n_dir: nombre de directions,
    // pour un réseau D2Q9 toujours égal à 3 sauf dans les coins où ne se
    // propage que 1 direction.

    // Rebond
    for (int q = 0; q < n_dir; q++) {
        N[IDX(dir[q], index, offset_N)] = N[IDX(dir_fs[q], index, offset_N)];
    }

    // Calcul de la pression
    cl_float rho = 0.0f;
    for (int q = 0; q < LATTICE_Q; q++) {
        rho += N[IDX(q, index, offset_N)];
    }

    //----------------------------------------------------------------------------

    // Ajout d'un terme source pour imposer rho = 1.0f
    for (int q = 0; q < n_dir; q++) {
        N[IDX(dir[q], index, offset_N)] += 0.5f * (1.0f - rho) * lattice_w[q];
    }
}

void borders(cl_float N[LATTICE_Q * NUMEL], int offset_N[LATTICE_Q]) {
    // Application des conditions limites à tous les bords du domaine de
    // simulation. 4 côtés
    for (int i = 1; i < SIZE_X - 1; i++) {
        int dir[3] = {2, 5, 6};
        int dir_fs[3] = {4, 8, 7};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_X - 1; j++) {
        int i = j + SIZE_X * (SIZE_Y - 1);

        int dir[3] = {4, 7, 8};
        int dir_fs[3] = {2, 6, 5};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_Y - 1; j++) {
        int i = 0 + SIZE_X * j;

        int dir[3] = {1, 5, 8};
        int dir_fs[3] = {3, 6, 7};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_Y - 1; j++) {
        int i = SIZE_X - 1 + SIZE_X * j;

        int dir[3] = {3, 7, 6};
        int dir_fs[3] = {1, 8, 5};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    // 4 coins
    int i, dir[1], dir_fs[1];

    i = 0 + SIZE_X * 0;
    dir[0] = 5;
    dir_fs[0] = 7;
    border_bc(N, i, dir, dir_fs, 1, offset_N);

    i = SIZE_X - 1 + SIZE_X * (SIZE_Y - 1);
    dir[0] = 7;
    dir_fs[0] = 5;
    border_bc(N, i, dir, dir_fs, 1, offset_N);

    i = SIZE_X - 1 + 0;
    dir[0] = 6;
    dir_fs[0] = 8;
    border_bc(N, i, dir, dir_fs, 1, offset_N);

    i = 0 + SIZE_X * (SIZE_Y - 1);
    dir[0] = 8;
    dir_fs[0] = 6;
    border_bc(N, i, dir, dir_fs, 1, offset_N);
}

int dirs[8][3] = {{5, 0, 0}, {7, 0, 0}, {2, 5, 6}, {4, 8, 7},
                  {6, 0, 0}, {8, 0, 0}, {1, 5, 8}, {3, 6, 7}};

// version préparée pour parallélisation : 1 seule boucle
void new_borders(cl_float *N, int *offset_N) {
    for (int i = 0; i < SIZE_X + SIZE_Y - 2; i++) {
        int index1, index2, idir, idir_fs, n_dir;

        if (i == 0) {
            n_dir = 1;

            index1 = 0 + SIZE_X * 0;
            idir = 0;
            idir_fs = 1;

            index2 = SIZE_X - 1 + SIZE_X * (SIZE_Y - 1);
        } else if (i < SIZE_X - 1) {
            n_dir = 3;

            index1 = i + SIZE_X * 0;
            idir = 2;
            idir_fs = 3;

            index2 = i + SIZE_X * (SIZE_Y - 1);
        } else if (i == SIZE_X - 1) {
            n_dir = 1;

            index1 = SIZE_X - 1 + SIZE_X * 0;
            idir = 4;
            idir_fs = 5;

            index2 = 0 + SIZE_X * (SIZE_Y - 1);
        } else {
            n_dir = 3;

            // y = 1, y < SIZE_Y - 1;
            index1 = 0 + (i - (SIZE_X - 1)) * SIZE_X;
            idir = 6;
            idir_fs = 7;

            index2 = SIZE_X - 1 + (i - (SIZE_X - 1)) * SIZE_X;
        }

        border_bc(N, index1, dirs[idir], dirs[idir_fs], n_dir, offset_N);
        border_bc(N, index2, dirs[idir_fs], dirs[idir], n_dir, offset_N);
    }
}

void cborders() {
    size_t global[] = {SIZE_X + SIZE_Y - 2};
    size_t local[] = {1};
    cl_int err = clEnqueueNDRangeKernel(pqueue, kernels[5], 1, NULL, global,
                                        local, 0, NULL, NULL);
    if (err) {
        printf("%d\n", err);
        exit(40);
    }
}

cl_float min(cl_float a, cl_float b) { return a < b ? a : b; }

void print_lines(int i, int k, cl_float *N, int *offsetN) {
    for (int x = i; x < k; ++x) {
        for (int q = 0; q < LATTICE_Q; ++q) {
            printf("%.10f ", N[IDX(q, x, offsetN)]);
        }
        printf("\n");
    }
}

int main() {
    cl_platform_id pplatforms;
    cl_device_id pdevice;
    cl_context pcontext;
    cl_int err;
    err = build_context(&pplatforms, &pdevice, &pcontext, &pqueue);
    if (err) {
        printf("could not build context (%d)", err);
        return 1;
    }
    char *kernel_names[] = {"init",  "collide_local", "stream",
                            "walls", "blows",         "borders"};
    err = build_kernels("flute.cl", NB_KERNEL, kernels, kernel_names, pcontext,
                        &pdevice);
    if (err) {
        printf("could not build kernels (%d)", err);
        return 35;
    }
    cl_float rho = 1.0f;
    cl_float u = 0.0f, v = 0.0f;
    int offset_N[LATTICE_Q];
    int period = 26;

    srand(0);
    cl_float *N;
    cl_float *rho_1;
    cl_float *u_1;
    cl_float *v_1;

    N = (cl_float *)malloc(NUMEL * LATTICE_Q * sizeof(cl_float));
    rho_1 = (cl_float *)malloc(NUMEL * sizeof(cl_float));
    u_1 = (cl_float *)malloc(NUMEL * sizeof(cl_float));
    v_1 = (cl_float *)malloc(NUMEL * sizeof(cl_float));

    cl_mem clN, cloffset_N;
    clN = clCreateBuffer(pcontext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE,
                         sizeof(cl_float) * NUMEL * LATTICE_Q, N, &err);
    if (err) return 33;
    cloffset_N =
        clCreateBuffer(pcontext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE,
                       sizeof(cl_int) * LATTICE_Q, offset_N, &err);
    if (err) return 34;

    clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &clN);
    clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &cloffset_N);
    clSetKernelArg(kernels[1], 0, sizeof(cl_mem), &clN);
    clSetKernelArg(kernels[1], 1, sizeof(cl_mem), &cloffset_N);
    clSetKernelArg(kernels[2], 0, sizeof(cl_mem), &cloffset_N);
    clSetKernelArg(kernels[3], 0, sizeof(cl_mem), &clN);
    clSetKernelArg(kernels[3], 1, sizeof(cl_mem), &cloffset_N);
    clSetKernelArg(kernels[4], 0, sizeof(cl_mem), &clN);
    clSetKernelArg(kernels[4], 1, sizeof(cl_mem), &cloffset_N);
    clSetKernelArg(kernels[5], 0, sizeof(cl_mem), &clN);
    clSetKernelArg(kernels[5], 1, sizeof(cl_mem), &cloffset_N);

    cl_float tau = 3.5e-5f * lattice_inv_cs2 + 0.5f;
    printf("rho: %f, u: %f, v: %f, tau: %f\n", rho, u, v, tau);
    // init(N, rho, u, v, offset_N);
    cinit(cloffset_N, offset_N);
    err = clEnqueueReadBuffer(pqueue, clN, CL_TRUE, 0,
                              NUMEL * LATTICE_Q * sizeof(cl_float), N, 0, NULL,
                              NULL);
    if (err) return 37;

    // for (int i = 0; i < NUMEL; ++i) {
    //     for (int q = 0; q < LATTICE_Q; ++q) {
    //         N[IDX(q, i, offset_N)] +=
    //             (((cl_float)rand()) / RAND_MAX) * 1e-4 - 5e-5;
    //     }
    // }
    err = clEnqueueWriteBuffer(pqueue, clN, CL_TRUE, 0,
                               NUMEL * LATTICE_Q * sizeof(cl_float), N, 0, NULL,
                               NULL);

    for (int t = 0; t < 10; ++t) {
        rho = 1.0f + min(t * 1.e-2f, 1.0f) * 0.0001f;
        // collide(N, tau, offset_N);

        ccollide();

        cstream();
        cwalls();
        cblows(rho);
        cborders();

        //  stream(offset_N);
        //  walls(wall, nb_wall, N, offset_N);
        //  blows(blow, nb_blow, N, offset_N, rho);
        // borders(N, offset_N);

        if (t % period == 0) {
            err = clEnqueueReadBuffer(pqueue, clN, CL_TRUE, 0,
                                      NUMEL * LATTICE_Q * sizeof(cl_float), N,
                                      0, NULL, NULL);
            calc_flow_properties_from_boltzmann(N, rho_1, u_1, v_1, offset_N);
            save(rho_1, u_1, v_1, t / period, SIZE_X, SIZE_Y);
        }
    }

    int index = 2;
    for (int q = 0; q < LATTICE_Q; ++q) {
        printf("%f, ", N[IDX(q, index, offset_N)]);
    }

    printf("\n");
    printf("%d", wall[0]);

    free(N);
    free(rho_1);
    free(u_1);
    free(v_1);

    return 0;
}