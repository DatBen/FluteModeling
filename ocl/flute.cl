#define LATTICE_D 2
#define LATTICE_Q 9
#define SIZE_X 12
#define SIZE_Y 10
#define NUMEL 120

__constant int nb_wall = 10;
__constant int wall[] = {25, 37, 42, 43, 44, 45, 49, 61, 73, 85};
__constant int nb_blow = 4;
__constant int blow[] = {38, 50, 62, 74};

__constant int lattice_c[LATTICE_Q][LATTICE_D] = {{0, 0},  {1, 0},   {0, 1},
                                                  {-1, 0}, {0, -1},  {1, 1},
                                                  {-1, 1}, {-1, -1}, {1, -1}};

__constant float lattice_w[LATTICE_Q] = {
    4.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f};

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

// __kernel void walls(global float *N, global int *offset_N) {
//   for (int i = 0; i < nb_wall; ++i) {
//     int index = wall[i];

//     for (int q = 0; q < LATTICE_Q; ++q) {
//       int q_bb = lattice_bounceback[q];
//       int index_orig =
//           (index - (lattice_c[q][0] + lattice_c[q][1] * SIZE_X)) % NUMEL;

//       cl_float temp = N[IDX(q_bb, index_orig, offset_N)];
//       N[IDX(q_bb, index_orig, offset_N)] = N[IDX(q, index, offset_N)];
//       N[IDX(q, index, offset_N)] = temp;
//     }
//   }
// }