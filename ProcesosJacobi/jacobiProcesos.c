#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

double *u, *f, *utmp;

struct process_info {
  int from;
  int to;
  double h2;
};

void process_subroutine(double h2, int from, int to) {
  int i;
  /* Old data in u; new data in utmp */
  for (i = from; i < to; ++i)
    utmp[i] = (u[i - 1] + u[i + 1] + h2 * f[i]) / 2;

  /* Old data in utmp; new data in u */
  for (i = from; i < to; ++i)
    u[i] = (utmp[i - 1] + utmp[i + 1] + h2 * f[i]) / 2;
}

void jacobi(int nsweeps, int size, int processes_ammount) {
  int i, sweep, process_idx, status;
  double h = 1.0 / size;
  double h2 = h * h;
  pid_t pidC;

  struct process_info processes_info[processes_ammount];

  utmp = (double *)mmap(NULL, (size + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  u = (double *)mmap(NULL, (size + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  f = (double *)mmap(NULL, (size + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  memset(u, 0, (size + 1) * sizeof(double));

  for (i = 0; i <= size; ++i)
    f[i] = i * h;

  utmp[0] = u[0];
  utmp[size] = u[size];

  for (process_idx = 0; process_idx < processes_ammount; process_idx++) {
    processes_info[process_idx].h2 = h2;
    processes_info[process_idx].from =
        process_idx * (size - 1) / processes_ammount;
    processes_info[process_idx].to =
        (process_idx + 1) * (size - 1) / processes_ammount;

    if (process_idx == 0) {
      processes_info[process_idx].from = 1;
    }
  }

  for (sweep = 0; sweep < nsweeps; sweep += 2) {
    for (process_idx = 0; process_idx < processes_ammount; process_idx++) {
      pidC = fork();
      if (pidC > 0)
        continue;
      else if (pidC == 0) {
        process_subroutine(h2, processes_info[process_idx].from,
                           processes_info[process_idx].to);
        exit(0);
      } else {
        printf("There was an error creating the process");
      }
    }
    for (process_idx = 0; process_idx < processes_ammount; process_idx++) {
      pidC = wait(&status);
    }
  }
}

void write_solution(int n, double *u, const char *fname) {
  int i;
  double h = 1.0 / n;
  FILE *fp = fopen(fname, "w+");
  for (i = 0; i <= n; ++i)
    fprintf(fp, "%g %g\n", i * h, u[i]);
  fclose(fp);
}

int main(int argc, char **argv) {
  int i, n, nsteps, processes_amount;
  double h;
  char *fname;
  struct timeval tstart, tend;

  /* Process arguments */
  n = (argc > 1) ? atoi(argv[1]) : 100;
  nsteps = (argc > 2) ? atoi(argv[2]) : 100;
  processes_amount = (argc > 3) ? atoi(argv[3]) : 4;
  fname = (argc > 4) ? argv[4] : NULL;
  h = 1.0 / n;

  /* Allocate and initialize arrays */
  u = (double *)mmap(NULL, (n + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  f = (double *)mmap(NULL, (n + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  utmp = (double *)mmap(NULL, (n + 1) * sizeof(double), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  gettimeofday(&tstart, NULL);
  jacobi(nsteps, n, processes_amount);
  gettimeofday(&tend, NULL);

  double exec_time =
      (tend.tv_sec - tstart.tv_sec) + 1e-6 * (tend.tv_usec - tstart.tv_usec);

  printf("%d,%d,%f\n", n, processes_amount, exec_time);

  if (fname)
    write_solution(n, u, fname);

  munmap(f, (n + 1) * sizeof(double));
  munmap(u, (n + 1) * sizeof(double));
  munmap(utmp, (n + 1) * sizeof(double));
  return 0;
}
