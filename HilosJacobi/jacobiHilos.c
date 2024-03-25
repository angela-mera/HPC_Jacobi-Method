#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Global variables
double *u, *f, *utmp;

struct thread_info { // Struct to hold the start and end indices of each thread
  int from;
  int to;
  double h2;
};

// Thread subroutine for the Jacobi method
void *thread_subroutine(void *threadarg) {
  struct thread_info *thread_info;
  int from, to, i;
  double h2;

  thread_info = (struct thread_info *)threadarg;
  h2 = thread_info->h2;
  from = thread_info->from;
  to = thread_info->to;

  // Perform the calculations using the given indices
  for (i = from; i < to; ++i)
    utmp[i] = (u[i - 1] + u[i + 1] + h2 * f[i]) / 2;

  
  for (i = from; i < to; ++i)
    u[i] = (utmp[i - 1] + utmp[i + 1] + h2 * f[i]) / 2;

  // Exit the thread
  pthread_exit(NULL);
}

// Main function for the Jacobi method
void jacobi(int nsweeps, int size, int threads_ammount) {
  // Calculate the step size and init the required data structures
  int i, sweep, thread_idx;
  double h = 1.0 / size;
  double h2 = h * h;

  struct thread_info threads_info[threads_ammount];
  pthread_t threads[threads_ammount];
  
  // Initialize the first and last values of the array
  utmp[0] = u[0];
  utmp[size] = u[size];

  // Perform the iterations
  for (sweep = 0; sweep < nsweeps; sweep += 2) {
    // Create the required number of threads and initialize their data
    for (thread_idx = 0; thread_idx < threads_ammount; thread_idx++) {

      threads_info[thread_idx].h2 = h2;
      threads_info[thread_idx].from = thread_idx * (size - 1) / threads_ammount;
      threads_info[thread_idx].to =
          (thread_idx + 1) * (size - 1) / threads_ammount;
      
      // Set the boundaries of the first and last threads
      if (thread_idx == 0) {
        threads_info[thread_idx].from = 1;
      }
      
      // Create the threads
      if (pthread_create(&threads[thread_idx], NULL, thread_subroutine,
                         (void *)&threads_info[thread_idx])) {
        printf("There was an error creating a thread\n");
        exit(-1);
      }
    }
    
     // Join the threads and collect the results
    for (thread_idx = 0; thread_idx < threads_ammount; thread_idx++) {
      if (pthread_join(threads[thread_idx], NULL)) {
        printf("There was an error joining a thread\n");
        exit(-1);
      }
    }
  }
}

// Function to write the solution to a file
void write_solution(int n, double *u, const char *fname) {
  int i;
  double h = 1.0 / n;
  FILE *fp = fopen(fname, "w+");
  for (i = 0; i <= n; ++i)
    fprintf(fp, "%g %g\n", i * h, u[i]);
  fclose(fp);
}

int main(int argc, char **argv) {
  int i, n, nsteps, threads_ammount;
  double h;
  char *fname;
  struct timeval tstart, tend;

  // Checking the input parameters and allocating the memory
  n = (argc > 1) ? atoi(argv[1]) : 100;
  nsteps = (argc > 2) ? atoi(argv[2]) : 100;
  threads_ammount = (argc > 3) ? atoi(argv[3]) : 4;
  fname = (argc > 4) ? argv[4] : NULL;
  h = 1.0 / n;

  // Allocate the required memory for u, f, and utmp
  u = (double *)malloc((n + 1) * sizeof(double));
  f = (double *)malloc((n + 1) * sizeof(double));
  utmp = (double *)malloc((n + 1) * sizeof(double));

  // Initializing the u array
  memset(u, 0, (n + 1) * sizeof(double));

  for (i = 0; i <= n; ++i)
    f[i] = i * h;

  
  gettimeofday(&tstart, NULL);
  jacobi(nsteps, n, threads_ammount);
  gettimeofday(&tend, NULL);

  double exec_time =
      (tend.tv_sec - tstart.tv_sec) + 1e-6 * (tend.tv_usec - tstart.tv_usec);
  
  printf("%d,%d,%f\n", n, threads_ammount, exec_time);
  
  if (fname)
    write_solution(n, u, fname);

  // Free the allocated memory and return
  free(f);
  free(u);
  free(utmp);
  
  return 0;
}
