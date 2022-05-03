#include <stdio.h>
#include "../include/utils.h"
#include "../include/flute.h"

// Définition du domaine de simulation

#define NUMEL (SIZE_X * SIZE_Y)

// Définition du réseau

#define LATTICE_D 2
#define LATTICE_Q 9

int IDX(int q, int index, int offset_N[LATTICE_Q])
{
    // return q * NUMEL + i;				// Sans propagation
    return q * NUMEL + (index + offset_N[q]) % NUMEL; // Avec propagation grace au tableau offset_N
}

int lattice_c[LATTICE_Q][LATTICE_D] = {
    {0, 0},
    {1, 0},
    {0, 1},
    {-1, 0},
    {0, -1},
    {1, 1},
    {-1, 1},
    {-1, -1},
    {1, -1}};

float lattice_w[LATTICE_Q] = {
    4.0f / 9.0f,
    1.0f / 9.0f,
    1.0f / 9.0f,
    1.0f / 9.0f,
    1.0f / 9.0f,
    1.0f / 36.0f,
    1.0f / 36.0f,
    1.0f / 36.0f,
    1.0f / 36.0f};

int lattice_bounceback[LATTICE_Q] = {
    0,
    3,
    4,
    1,
    2,
    7,
    8,
    5,
    6};

float lattice_inv_cs2 = 3.0f;

// Collision

void calc_flow_properties_from_boltzmann_local(float N[LATTICE_Q * NUMEL], float *rho, float *u, float *v, int index, int offset_N[LATTICE_Q])
{
    // Une fonction qui calcule les propriétés locales de l'écoulement à partir des distributions de Boltzmann
    *rho = 0.0f;
    *u = 0.0f;
    *v = 0.0f;
    for (int q = 0; q < LATTICE_Q; ++q)
    {
        *rho += N[IDX(q, index, offset_N)];
        *u += N[IDX(q, index, offset_N)] * lattice_c[q][0];
        *v += N[IDX(q, index, offset_N)] * lattice_c[q][1];
    }
    *u /= *rho;
    *v /= *rho;
}

void calc_flow_properties_from_boltzmann(float N[LATTICE_Q * NUMEL], float rho[NUMEL], float u[NUMEL], float v[NUMEL], int offset_N[LATTICE_Q])
{
    for (int i = 0; i < NUMEL; ++i)
    {
        calc_flow_properties_from_boltzmann_local(N, rho + i, u + i, v + i, i, offset_N);
    }
}

void calc_equilibrium_from_flow_properties_local(float rho, float u, float v, float Neq[LATTICE_Q])
{
    // Une fontion qui calcule la distribution à l'équilibre correspondante aux propriétés d'écoulement

    float v2 = u * u + v * v;
    for (int q = 0; q < LATTICE_Q; ++q)
    {
        float vc = u * lattice_c[q][0] + v * lattice_c[q][1];
        Neq[q] = rho * lattice_w[q] * (1.0f + lattice_inv_cs2 * (vc + 0.5f * (vc * vc * lattice_inv_cs2 - v2)));
    }
}

void collide_local(float N[LATTICE_Q * NUMEL], float tau, int index, int offset_N[LATTICE_Q])
{
    float Neq[LATTICE_Q];
    float rho, u, v;

    calc_flow_properties_from_boltzmann_local(N, &rho, &u, &v, index, offset_N);

    calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);

    for (int q = 0; q < LATTICE_Q; ++q)
    {
        N[IDX(q, index, offset_N)] -= (N[IDX(q, index, offset_N)] - Neq[q]) / tau;
    }
}

void collide(float N[LATTICE_Q * NUMEL], float tau, int offset_N[LATTICE_Q])
{
    for (int index = 0; index < NUMEL; ++index)
    {
        collide_local(N, tau, index, offset_N);
    }
}

// Initialisation

void init(float N[LATTICE_Q * NUMEL], float rho, float u, float v, int offset_N[LATTICE_Q])
{
    float Neq[LATTICE_Q];
    calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);

    for (int q = 0; q < LATTICE_Q; ++q)
    {
        offset_N[q] = 0;
    }

    for (int index = 0; index < NUMEL; ++index)
    {
        for (int q = 0; q < LATTICE_Q; ++q)
        {
            N[IDX(q, index, offset_N)] = Neq[q];
        }
    }
}

// Propagation

void stream(int offset_N[LATTICE_Q])
{
    for (int q = 1; q < LATTICE_Q; ++q)
    {
        /**** Attention ****/
        // offset_N[q] = (offset_N[q] + lattice_c[q][0] + lattice_c[q][1] * SIZE_X) % NUMEL;
        //  Deux erreurs dans la ligne ci-dessus:
        //  1. le signe: décaler toutes les valeurs dans un sens (positif par exemple) revent à décaler le zéro (offset) dans l'autre sens (négatif).
        //  2. % n'est pas l'opérateur modulo et se comporte différemment pour les valeurs négatives, or que l'on ajoute ou soustraie l'offset, c peut avoir des composantes négatives ou positives, il faut donc gérer correctement les cas négatifs.
        //  Les 5 lignes ci-dessous sont un peu plus longues mais sont justes :
        offset_N[q] = (offset_N[q] - lattice_c[q][0] - lattice_c[q][1] * SIZE_X);
        if (offset_N[q] >= NUMEL)
            offset_N[q] -= NUMEL;
        if (offset_N[q] < 0)
            offset_N[q] += NUMEL;
    }
}

// Conditions limites

void walls(int wall[], int nb_wall, float N[LATTICE_Q * NUMEL], int offset_N[LATTICE_Q])
{
    /* Parois */
    for (int i = 0; i < nb_wall; ++i)
    {
        int index = wall[i];

        for (int q = 0; q < LATTICE_Q; ++q)
        {
            int q_bb = lattice_bounceback[q];
            int index_orig = (index - (lattice_c[q][0] + lattice_c[q][1] * SIZE_X)) % NUMEL;

            float temp = N[IDX(q_bb, index_orig, offset_N)];
            N[IDX(q_bb, index_orig, offset_N)] = N[IDX(q, index, offset_N)];
            N[IDX(q, index, offset_N)] = temp;
        }
    }
}

void blows(int blow[], int nb_blow, float N[LATTICE_Q * NUMEL], int offset_N[LATTICE_Q], float p)
{
    int dir[3] = {1, 5, 8};
    int dir_fs[3] = {3, 6, 7}; // free slip: glissement

    for (int i = 0; i < nb_blow; ++i)
    {
        int index = blow[i];

        for (int q = 0; q < 3; ++q)
        {
            N[IDX(dir[q], index, offset_N)] = N[IDX(dir_fs[q], index, offset_N)];
        }

        float rho = 0.0f;
        for (int q = 0; q < LATTICE_Q; ++q)
        {
            rho += N[IDX(q, index, offset_N)];
        }

        for (int q = 0; q < 3; ++q)
        {
            N[IDX(dir[q], index, offset_N)] += (p - rho) * lattice_w[q];
        }
    }
}

void border_bc(float N[LATTICE_Q * NUMEL], int index, int dir[], int dir_fs[], int n_dir, int offset_N[LATTICE_Q])
{
    // dir: directions entrantes. Pour la condition limite considérées, indices des vecteurs tels que c_i . n_bc > 0 (avec n_bc, vecteur normal pointant vers l'intérieur du domaine).
    // Par exemple pour une condtion limite en x = 0, la normale entrante est e_x, donc dir = {1, 5, 8}.
    // dir_fs: direction de rebond des directions entrantes (ici rebond seulement dans la direction normale pour laisser le fluide glisser dans la direction tangentielle).
    // n_dir: nombre de directions, pour un réseau D2Q9 toujours égal à 3 sauf dans les coins où ne se propage que 1 direction.

    // Rebond
    for (int q = 0; q < n_dir; q++)
    {
        N[IDX(dir[q], index, offset_N)] = N[IDX(dir_fs[q], index, offset_N)];
    }

    // Calcul de la pression
    float rho = 0.0f;
    for (int q = 0; q < LATTICE_Q; q++)
    {
        rho += N[IDX(q, index, offset_N)];
    }

    //----------------------------------------------------------------------------

    // Ajout d'un terme source pour imposer rho = 1.0f
    for (int q = 0; q < n_dir; q++)
    {
        N[IDX(dir[q], index, offset_N)] += 0.5f * (1.0f - rho) * lattice_w[q];
    }
}

void borders(float N[LATTICE_Q * NUMEL], int offset_N[LATTICE_Q])
{
    // Application des conditions limites à tous les bords du domaine de simulation.
    // 4 côtés
    for (int i = 1; i < SIZE_X - 1; i++)
    {
        int dir[3] = {2, 5, 6};
        int dir_fs[3] = {4, 8, 7};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_X - 1; j++)
    {
        int i = j + SIZE_X * (SIZE_Y - 1);

        int dir[3] = {4, 7, 8};
        int dir_fs[3] = {2, 6, 5};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_Y - 1; j++)
    {
        int i = 0 + SIZE_X * j;

        int dir[3] = {1, 5, 8};
        int dir_fs[3] = {3, 6, 7};
        border_bc(N, i, dir, dir_fs, 3, offset_N);
    }

    for (int j = 1; j < SIZE_Y - 1; j++)
    {
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

// void new_borders(float N[LATTICE_Q * NUMEL], int offset_N[LATTICE_Q])
// {
// 	for (int index = 0; index < SIZE_X; ++index)
// 	{
//
// 	}
// }

float min(float a, float b)
{
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

int main()
{

    float rho = 1.0f;
    float u = 0.0f, v = 0.0f;
    int offset_N[LATTICE_Q];
    int period = 26;

    // float N[LATTICE_Q * NUMEL];
    // float rho_1[NUMEL];
    // float u_1[NUMEL];
    // float v_1[NUMEL];

    float *N;
    float *rho_1;
    float *u_1;
    float *v_1;

    N = (float *)malloc(NUMEL * LATTICE_Q * sizeof(float));
    rho_1 = (float *)malloc(NUMEL * sizeof(float));
    u_1 = (float *)malloc(NUMEL * sizeof(float));
    v_1 = (float *)malloc(NUMEL * sizeof(float));

    float tau = 3.5e-5f * lattice_inv_cs2 + 0.5f;
    printf("rho: %f, u: %f, v: %f, tau: %f\n", rho, u, v, tau);
    init(N, rho, u, v, offset_N);

    // for (int q = 0; q < LATTICE_Q; ++q) {
    // 	N[IDX(q, (SIZE_Y / 2) * SIZE_X + SIZE_X / 2, offset_N)] *= 1.1f;
    // 	N[IDX(q, (SIZE_Y / 2 + 1) * SIZE_X + SIZE_X / 2, offset_N)] *= 1.1f;
    // 	N[IDX(q, (SIZE_Y / 2) * SIZE_X + SIZE_X / 2 + 1, offset_N)] *= 1.1f;
    // 	N[IDX(q, (SIZE_Y / 2 + 1) * SIZE_X + SIZE_X / 2 + 1, offset_N)] *= 1.1f;
    // }

    for (int t = 0; t < 100000; ++t)
    {
        rho = 1.0f + min(t * 1.e-2f, 1.0f) * 0.0001f;
        collide(N, tau, offset_N);
        stream(offset_N);
        walls(wall, nb_wall, N, offset_N);
        blows(blow, nb_blow, N, offset_N, rho);
        borders(N, offset_N);

        if (t % period == 0)
        {
            calc_flow_properties_from_boltzmann(N, rho_1, u_1, v_1, offset_N);
            save(rho_1, u_1, v_1, t / period, SIZE_X, SIZE_Y);
        }
    }

    int index = 2;
    for (int q = 0; q < LATTICE_Q; ++q)
    {
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