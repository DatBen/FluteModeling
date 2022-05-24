#define LATTICE_D 2
#define LATTICE_Q 9
#define SIZE_X 12
#define SIZE_Y 10
#define NUMEL 120

__constant int nb_wall = 10;
__constant int wall[] = {25, 37, 42, 43, 44, 45, 49, 61, 73, 85};
__constant int nb_blow = 4;
__constant int blow[] = {38, 50, 62, 74};
__constant int dir[3] = {1, 5, 8};
__constant int dir_fs[3] = {3, 6, 7};
__constant int dirs[8][3] = {{5, 0, 0}, {7, 0, 0}, {2, 5, 6}, {4, 8, 7},
                             {6, 0, 0}, {8, 0, 0}, {1, 5, 8}, {3, 6, 7}};

__constant int lattice_c[LATTICE_Q][LATTICE_D] = {{0, 0},  {1, 0},   {0, 1},
                                                  {-1, 0}, {0, -1},  {1, 1},
                                                  {-1, 1}, {-1, -1}, {1, -1}};

__constant float lattice_w[LATTICE_Q] = {
    4.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f};
__constant int lattice_bounceback[LATTICE_Q] = {0, 3, 4, 1, 2, 7, 8, 5, 6};

__constant float lattice_inv_cs2 = 3.0f;
__constant float tau = 3.5e-5f * 3.0f + 0.5f;

int IDX(int q, int index, global int *offset_N) {
  return q * NUMEL + ((index + offset_N[q]) % NUMEL);
}

void calc_equilibrium_from_flow_properties_local(float rho, float u, float v,
                                                 float Neq[LATTICE_Q]) {
  float v2 = u * u + v * v;
  for (int q = 0; q < LATTICE_Q; ++q) {
    float vc = u * lattice_c[q][0] + v * lattice_c[q][1];
    Neq[q] = rho * lattice_w[q] *
             (1.0f +
              lattice_inv_cs2 * (vc + 0.5f * (vc * vc * lattice_inv_cs2 - v2)));
  }
}

__kernel void init(global float *N, global int *offset_N) {
  int index = get_global_id(0);
  float rho, u, v;
  rho = 1.0f;
  u = 0.0f;
  v = 0.0f;
  float Neq[LATTICE_Q];
  calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);

  for (int q = 0; q < LATTICE_Q; ++q) {
    N[IDX(q, index, offset_N)] = Neq[q];
  }
}

void calc_flow_properties_from_boltzmann_local(global float *N, float *rho,
                                               float *u, float *v, int index,
                                               global int *offset_N) {
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

__kernel void collide_local(global float *N, global int *offset_N) {
  float Neq[LATTICE_Q];
  float rho, u, v;
  int index = get_global_id(0);
  calc_flow_properties_from_boltzmann_local(N, &rho, &u, &v, index, offset_N);
  calc_equilibrium_from_flow_properties_local(rho, u, v, Neq);
  for (int q = 0; q < LATTICE_Q; ++q) {
    N[IDX(q, index, offset_N)] -= (N[IDX(q, index, offset_N)] - Neq[q]) / tau;
  }
}

__kernel void stream(global int *offset_N) {
  for (int q = 1; q < LATTICE_Q; ++q) {
    offset_N[q] = (offset_N[q] - lattice_c[q][0] - lattice_c[q][1] * SIZE_X);
    if (offset_N[q] >= NUMEL)
      offset_N[q] -= NUMEL;
    if (offset_N[q] < 0)
      offset_N[q] += NUMEL;
  }
}

__kernel void walls(global float *N, global int *offset_N) {
  int index0 = get_global_id(0);
  int index = wall[index0];
  for (int q = 0; q < LATTICE_Q; ++q) {
    int q_bb = lattice_bounceback[q];
    int index_orig =
        (index - (lattice_c[q][0] + lattice_c[q][1] * SIZE_X)) % NUMEL;

    float temp = N[IDX(q_bb, index_orig, offset_N)];
    N[IDX(q_bb, index_orig, offset_N)] = N[IDX(q, index, offset_N)];
    N[IDX(q, index, offset_N)] = temp;
  }
}

__kernel void blows(global float *N, global int *offset_N, float p) {

  int index0 = get_global_id(0);
  int index = blow[index0];
  for (int q = 0; q < 3; ++q) {
    N[IDX(dir[q], index, offset_N)] = N[IDX(dir_fs[q], index, offset_N)];
  }

  float rho = 0.0f;
  for (int q = 0; q < LATTICE_Q; ++q) {
    rho += N[IDX(q, index, offset_N)];
  }

  for (int q = 0; q < 3; ++q) {
    N[IDX(dir[q], index, offset_N)] += (p - rho) * lattice_w[q];
  }
}

void border_bc(global float *N, int index, int idir, int idir_fs, int n_dir,
               global int *offset_N) {
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
  int dir_l[] = {0, 0, 0};
  int dir_fs_l[] = {0, 0, 0};
  for (int i = 0; i < 3; i++) {
    dir_l[i] = dirs[idir][i];
    dir_fs_l[i] = dirs[idir_fs][i];
  }

  for (int q = 0; q < n_dir; q++) {
    N[IDX(dir_l[q], index, offset_N)] = N[IDX(dir_fs_l[q], index, offset_N)];
  }

  // Calcul de la pression
  float rho = 0.0f;
  for (int q = 0; q < LATTICE_Q; q++) {
    rho += N[IDX(q, index, offset_N)];
  }

  //----------------------------------------------------------------------------

  // Ajout d'un terme source pour imposer rho = 1.0f
  for (int q = 0; q < n_dir; q++) {
    N[IDX(dir[q], index, offset_N)] += 0.5f * (1.0f - rho) * lattice_w[q];
  }
}

__kernel void borders(global float *N, global int *offset_N) {
  int index_c = get_global_id(0);
  int index1, index2, idir, idir_fs, n_dir;

  if (index_c == 0) {
    n_dir = 1;

    index1 = 0 + SIZE_X * 0;
    idir = 0;
    idir_fs = 1;

    index2 = SIZE_X - 1 + SIZE_X * (SIZE_Y - 1);
  } else if (index_c < SIZE_X - 1) {
    n_dir = 3;

    index1 = index_c + SIZE_X * 0;
    idir = 2;
    idir_fs = 3;

    index2 = index_c + SIZE_X * (SIZE_Y - 1);
  } else if (index_c == SIZE_X - 1) {
    n_dir = 1;

    index1 = SIZE_X - 1 + SIZE_X * 0;
    idir = 4;
    idir_fs = 5;

    index2 = 0 + SIZE_X * (SIZE_Y - 1);
  } else {
    n_dir = 3;

    // y = 1, y < SIZE_Y - 1;dirs[idir]
    index1 = 0 + (index_c - (SIZE_X - 1)) * SIZE_X;
    idir = 6;
    idir_fs = 7;

    index2 = SIZE_X - 1 + (index_c - (SIZE_X - 1)) * SIZE_X;
  }

  border_bc(N, index1, idir, idir_fs, n_dir, offset_N);
  border_bc(N, index2, idir_fs, idir, n_dir, offset_N);
}
