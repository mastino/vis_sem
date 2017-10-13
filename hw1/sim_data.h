// simulation data stuff for vis

// Includes to make visit work
#include <VisItControlInterface_V2.h>
#include <VisItDataInterface_V2.h> 

// definitions
#define VISIT_COMMAND_PROCESS 0
#define VISIT_COMMAND_SUCCESS 1
#define VISIT_COMMAND_FAILURE 2


/* SIMPLE SIMULATION SKELETON */
typedef struct
{
  double* data;
  int     cycle;
  double  time;
  int     runMode;
  char    done;

#ifdef PARALLEL
  int par_rank;
  int par_size;
#endif

} simulation_data;

