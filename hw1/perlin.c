/*
 * perlin.c
 * 2d perlin noise thingy for integration with visit

 * I took this code from https://gist.github.com/nowl/828013
 * and modified it to suit my needs

 * Brian Gravelle
 * Oct 2017

*/

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include "sim_data.h"

#ifndef N
#define N 10000
#endif
#define NUM_CYCLES 100

#define DIMS 2
#define PTS_PER_DIM 5000

static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                     185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                     9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                     70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                     203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                     164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                     228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                     232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                     193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                     101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                     135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                     114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219};



int    noise2(int x, int y);
double lin_inter(double x, double y, double s);
double smooth_inter(double x, double y, double s);
double noise2d(double x, double y);
double perlin2d(double x, double y, double freq, int depth);
void   simulation_data_ctor(simulation_data *sim);
void   main_loop(simulation_data *sim);
void   simulate_one_timestep(simulation_data *sim);
void   write_vis_dump(simulation_data *sim);

void SetMeshMetaData(void *cbdata);

visit_handle SimGetMetaData(void *cbdata);
visit_handle SimGetVariable(int domain, const char *name, void *cbdata);
visit_handle VisItGetMesh(int domain, const char *name, void *data);
void ControlCommandCallback(const char *cmd, const char *args, void *cbdata);


int main(int argc, char *argv[]) {


  simulation_data sim;
  simulation_data_ctor(&sim);
  double* area = (double*)malloc(N * N * sizeof(double));
  sim.data = area;

  VisItSetupEnvironment();
  char path_to_binary[1024];
  strcpy(path_to_binary, CUR_PATH); // CUR_PATH == macro from Makefile
  VisItInitializeSocketAndDumpSimFile("Brian_Perlin_Noise", "meh", path_to_binary, NULL, NULL, NULL);

  //TODO is this where these go?
  /* Register data access callbacks */
  VisItSetGetMetaData(SimGetMetaData, (void*)&sim);
  SetMeshMetaData((void*)&sim);
  VisItSetGetMesh(VisItGetMesh, (void*)&sim);

  //VisItSetGetVariable(SimGetVariable, (void*)&sim);

  //read_input_deck(&sim);
  main_loop(&sim);

  //simulation_data_dtor(&sim);

  free(area);

  return 0;

}


void   simulation_data_ctor(simulation_data *sim) {
  sim->cycle   = 0;
  sim->time    = 0;
  sim->runMode = VISIT_SIMMODE_STOPPED;
  sim->done    = 0;
}

void main_loop(simulation_data *sim) {

  int blocking, visitstate, err = 0;

  do {

    blocking = (sim->runMode == VISIT_SIMMODE_RUNNING) ? 0 : 1;
    /* Get input from VisIt or timeout so the simulation can run. */
    visitstate = VisItDetectInput(blocking, -1);
    /* Do different things depending on the output from
    VisItDetectInput. */
    if(visitstate <= -1) {
      fprintf(stderr, "Can’t recover from error!\n");
      err = 1;
    } else if(visitstate == 0) {
      /* There was no input from VisIt, return control to sim. */
      simulate_one_timestep(sim);
    } else if(visitstate == 1) {
      /* VisIt is trying to connect to sim. */
      if(VisItAttemptToCompleteConnection()) {
        fprintf(stderr, "VisIt connected\n");
        VisItSetCommandCallback(ControlCommandCallback, (void*)sim);
      }
      else
        fprintf(stderr, "VisIt did not connect\n");
    } else if(visitstate == 2) {
      /* VisIt wants to tell the engine something. */
      sim->runMode = VISIT_SIMMODE_STOPPED;
      if(!VisItProcessEngineCommand()) {
        /* Disconnect on an error or closed connection. */
        VisItDisconnect();
        /* Start running again if VisIt closes. */
        sim->runMode = VISIT_SIMMODE_RUNNING;
      }
    }

  } while(!sim->done);

}



visit_handle SimGetMetaData(void *cbdata) {
  visit_handle md = VISIT_INVALID_HANDLE;
  simulation_data *sim = (simulation_data *)cbdata;
  
  /* Create metadata with no variables. */
  if(VisIt_SimulationMetaData_alloc(&md) == VISIT_OKAY) {
    
    if(sim->runMode == VISIT_SIMMODE_STOPPED)
      VisIt_SimulationMetaData_setMode(md, VISIT_SIMMODE_STOPPED);
    else
      VisIt_SimulationMetaData_setMode(md, VISIT_SIMMODE_RUNNING);
    
    VisIt_SimulationMetaData_setCycleTime(md, sim->cycle, sim->time);
  }

  return md;
}

// Not the actual mesh because that would make sense
// This gives metadata about the mesh
void SetMeshMetaData(void *cbdata) {
  
  visit_handle md = VISIT_INVALID_HANDLE;
  simulation_data *sim = (simulation_data *)cbdata;

  visit_handle m1 = VISIT_INVALID_HANDLE;
  visit_handle m2 = VISIT_INVALID_HANDLE;

  //TODO fix and figure out if this is correct
  if(VisIt_VariableMetaData_alloc(&md) == VISIT_OKAY) {
    VisIt_MeshMetaData_setName(m1, "area");
    VisIt_MeshMetaData_setMeshType(m1, VISIT_MESHTYPE_RECTILINEAR);
    VisIt_MeshMetaData_setTopologicalDimension(m1, 2);
    VisIt_MeshMetaData_setSpatialDimension(m1, 2);
    VisIt_SimulationMetaData_addMesh(md, m1);
  }

}

// still not sure what this is good for
// TODO figure out
visit_handle SimGetVariable(int domain, const char *name, void *cbdata) {

    visit_handle h = VISIT_INVALID_HANDLE;
    int nComponents = 1, nTuples = N*N;
  
    if(VisIt_VariableData_alloc(&h) == VISIT_OKAY) {

        if(strcmp(name, "speed") == 0) {

            static float *var_data = new float[nTuples];
          
            for (int i = 0; i < nTuples; i++) {
              var_data[i] = 5;
            }
 
            VisIt_VariableData_setDataF(h, VISIT_OWNER_SIM, nComponents, nTuples, var_data);
        }
    }
    return h;
}

// this is where the magic happens
visit_handle VisItGetMesh(int domain, const char *name, void *data) {

  visit_handle mesh = VISIT_INVALID_HANDLE;
  visit_handle h = VISIT_INVALID_HANDLE;

  float rmesh_x[N];
  float rmesh_y[N];
  float c = 0.0;
  for (int i = 0; i < N; i++) {
    rmesh_x[i] = rmesh_y[i] = c++;
  }

  if(strcmp(name, "area") == 0) {
    /* Allocate a rectilinear mesh. */
    if(VisIt_RectilinearMesh_alloc(&mesh) == VISIT_OKAY) {
      if(VisIt_RectilinearMesh_alloc(&h) != VISIT_ERROR) {
          visit_handle hxc, hyc;
          VisIt_VariableData_alloc(&hxc);
          VisIt_VariableData_alloc(&hyc);
          VisIt_VariableData_setDataF(hxc, VISIT_OWNER_SIM, 1, N, rmesh_x);
          VisIt_VariableData_setDataF(hyc, VISIT_OWNER_SIM, 1, N, rmesh_y);
          VisIt_RectilinearMesh_setCoordsXY(h, hxc, hyc);
      }
    }
  }
  // where you have lots of meshes
  // } else if(strcmp(name, "mesh3d") == 0) {
  //   /* Allocate a curvilinear mesh. */
  //   if(VisIt_CurvilinearMesh_alloc(&mesh) == VISIT_OKAY) {
  //     /* Fill in the attributes of the CurvilinearMesh. */
  //   }
  // }

  return mesh;
}

void ControlCommandCallback(const char *cmd, const char *args, void *cbdata) {
  simulation_data *sim = (simulation_data *)cbdata;
  if(strcmp(cmd, "halt") == 0)
  sim->runMode = VISIT_SIMMODE_STOPPED;
  else if(strcmp(cmd, "step") == 0)
  simulate_one_timestep(sim);
  else if(strcmp(cmd, "run") == 0)
  sim->runMode = VISIT_SIMMODE_RUNNING;
}

void simulate_one_timestep(simulation_data *sim) {

  int x, y;
  double * area = sim->data;

  for(y = 0; y < N; y++)
    for(x = 0; x < N; x++)
      perlin2d(area[x], area[y], 0.1, 4);

  sim->cycle++;
  sim->done = sim->cycle > NUM_CYCLES;
  write_vis_dump(sim);

}

void write_vis_dump(simulation_data *sim) {
  printf("Should be writing data here.\n");
}

int noise2(int x, int y) {
  int tmp = hash[(y + SEED) % 256];
  return hash[(tmp + x) % 256];
}

double lin_inter(double x, double y, double s) {
  return x + s * (y-x);
}

double smooth_inter(double x, double y, double s) {
  return lin_inter(x, y, s * s * (3-2*s));
}

double noise2d(double x, double y) {
  int x_int = x;  
  int y_int = y;
  double x_frac = x - x_int;
  double y_frac = y - y_int;
  int s = noise2(x_int, y_int);
  int t = noise2(x_int+1, y_int);
  int u = noise2(x_int, y_int+1);
  int v = noise2(x_int+1, y_int+1);
  double low = smooth_inter(s, t, x_frac);
  double high = smooth_inter(u, v, x_frac);
  return smooth_inter(low, high, y_frac);
}

double perlin2d(double x, double y, double freq, int depth) {
  double xa = x*freq;
  double ya = y*freq;
  double amp = 1.0;
  double fin = 0;
  double div = 0.0;

  int i;
  for(i=0; i<depth; i++) {
    div += 256 * amp;
    fin += noise2d(xa, ya) * amp;
    amp /= 2;
    xa *= 2;
    ya *= 2;
  }

  return fin/div;
}