#define SIZE_X 200
#define SIZE_Y 100
#define LATTICE_D 2 
#define LATTICE_Q 9
#define NUMEL 20000
__constant int nb_wall = 370;
__constant int wall[] = {8405, 8406, 8407, 8408, 8409, 8605, 8609, 8610, 8805, 8810, 8811, 9005, 9011, 9012, 9033, 9044, 9045, 9046, 9047, 9048, 9049, 9050, 9051, 9052, 9053, 9054, 9055, 9056, 9057, 9058, 9059, 9060, 9061, 9062, 9063, 9064, 9065, 9066, 9067, 9068, 9069, 9070, 9071, 9072, 9073, 9074, 9075, 9076, 9077, 9078, 9079, 9080, 9081, 9082, 9083, 9084, 9085, 9086, 9087, 9088, 9089, 9090, 9205, 9212, 9213, 9232, 9233, 9243, 9244, 9245, 9246, 9247, 9248, 9249, 9250, 9251, 9252, 9253, 9254, 9255, 9256, 9257, 9258, 9259, 9260, 9261, 9262, 9263, 9264, 9265, 9266, 9267, 9268, 9269, 9270, 9271, 9272, 9273, 9274, 9275, 9276, 9277, 9278, 9279, 9280, 9281, 9282, 9283, 9284, 9285, 9286, 9287, 9288, 9289, 9290, 9405, 9413, 9414, 9415, 9416, 9417, 9418, 9419, 9420, 9421, 9422, 9423, 9424, 9425, 9426, 9427, 9428, 9429, 9430, 9431, 9432, 9442, 9443, 9444, 9445, 9446, 9447, 9448, 9449, 9450, 9451, 9452, 9453, 9454, 9455, 9456, 9457, 9458, 9459, 9460, 9461, 9462, 9463, 9464, 9465, 9466, 9467, 9468, 9469, 9470, 9471, 9472, 9473, 9474, 9475, 9476, 9477, 9478, 9479, 9480, 9481, 9482, 9483, 9484, 9485, 9486, 9487, 9488, 9489, 9490, 9605, 9641, 9642, 9643, 9644, 9645, 9646, 9647, 9648, 9649, 9650, 9651, 9652, 9653, 9654, 9655, 9656, 9657, 9658, 9659, 9660, 9661, 9662, 9663, 9664, 9665, 9666, 9667, 9668, 9669, 9670, 9671, 9672, 9673, 9674, 9675, 9676, 9677, 9678, 9679, 9680, 9681, 9682, 9683, 9684, 9685, 9686, 9687, 9688, 9689, 9690, 9691, 9805, 9891, 9892, 10005, 10092, 10093, 10205, 10293, 10294, 10405, 10494, 10495, 10605, 10613, 10614, 10615, 10616, 10617, 10618, 10619, 10620, 10621, 10622, 10623, 10624, 10625, 10626, 10627, 10628, 10629, 10630, 10631, 10632, 10695, 10805, 10812, 10813, 10832, 10833, 11005, 11011, 11012, 11033, 11205, 11210, 11211, 11233, 11405, 11409, 11410, 11433, 11605, 11606, 11607, 11608, 11609, 11633, 11833, 12033, 12233, 12433, 12495, 12633, 12694, 12695, 12833, 12893, 12894, 13033, 13092, 13093, 13233, 13291, 13292, 13433, 13434, 13435, 13436, 13437, 13438, 13439, 13440, 13441, 13442, 13443, 13444, 13445, 13446, 13447, 13448, 13449, 13450, 13451, 13452, 13453, 13454, 13455, 13456, 13457, 13458, 13459, 13460, 13461, 13462, 13463, 13464, 13465, 13466, 13467, 13468, 13469, 13470, 13471, 13472, 13473, 13474, 13475, 13476, 13477, 13478, 13479, 13480, 13481, 13482, 13483, 13484, 13485, 13486, 13487, 13488, 13489, 13490, 13491};
__constant int nb_blow = 15;
__constant int blow[]={8606,8806,9006,9206,9406,9606,9806,10006,10206,10406,10606,10806,11006,11206,11406};


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
