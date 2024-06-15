#include <stdio.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h> 
#include <stdlib.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

//int avg = 0; /* Global variables to hold the final average */
//use flag sig_atomic_t
volatile sig_atomic_t avg = 0; 
//I'm not sure if this shoudl be initialized to 0 or not? 
volatile sig_atomic_t average = 0; 
/* function to initialize an array of integers with random values */
void initialize(int*);

/* Wrapper function prototypes for the system calls */
void unix_error(const char *msg); //complete
pid_t Fork(); //complete
pid_t Wait(int *status); // complete
pid_t Waitpid(pid_t pid, int *status, int options); //complete
int Sigqueue(pid_t pid, int signum, union sigval value); //complete
int Sigemptyset(sigset_t *set); //complete
int Sigfillset(sigset_t *set); //complete
int Sigaction(int signum, const struct sigaction *new_act, struct sigaction *old_act); // complete
int Sigprocmask(int how, const sigset_t *set, sigset_t *oldset); //complete
ssize_t Write(int d, const void *buffer, size_t nbytes); //complete
typedef void handler_t;
handler_t *Signal(int signum, handler_t *handler); //complete


/* Prototype of the handler */
void sigusr1_handler(int sig, siginfo_t *value, void *ucontext);

/* main function */
int main(){
  int A[N];
  initialize(A);
  /* install a portable handler for SIGUSR1 using the wrapper function Signal */
  Signal(SIGUSR1, sigusr1_handler);
  /* print the message for installing SIGUSR1 handler*/
  fprintf(stderr, "Parent process %d installing SIGUSR1 handler\n", getpid()); //getpid is for current, getppid is for parent
  /* create (P child processes) to calculate a partial average and send the signal SIGUSR1 to the parent*/
  //pid_t child_pid[P]; 
  // I'm assuming we do not have to initialize P
  for (int i = 0; i < P; i++) {
    //before starting child process, need to fork to create the child process
    pid_t pid = Fork(); //child process created

    if (pid == 0){
      int first = i * (N / P);
      int last = (i * (N/P)) + (N/P);
      //declare the partial sum 
      int partialSum = 0;
      for (int i = first; i <= last; i++) {
        partialSum = partialSum + A[i]; //this array is created with int
      }
      //here, the average can be calculated
      int val = partialSum / (N / P);

      fprintf(stderr, "Child process %d finding the average from %d to %d\n", getpid(), first, last - 1);

      //send signal
      //AS described in program description
      union sigval value;
      value.sival_int = val; //partialAverage is the same as val
      fprintf(stderr, "Child process %d sending SIGUSR1 to parent process with the partial average %d\n", getpid(), val);
      sleep(i); // need to allow handler to execute
      Sigqueue(getppid(), SIGUSR1, value); // use sigqueue to send the signal
      exit(0);

    }

    //here, the average can be calculated
    //int partialAverage = partialSum / (N / P);
    //printf("Child process %d finding the average from %d to %d\n", getpid(), start, end - 1);
  }

  //signal sending
  /* reap the (P) children */
  while(average < P){
    sleep(1);
  }

  for (int i = 0; i < P; i++) {
    int finishedChildProcesses; // to hold finishedChildProcesses and will keep count
    pid_t child = Wait(&finishedChildProcesses);
    if(child > 0){
      //Source for WIFEXITED: https://stackoverflow.com/questions/47441871/why-should-we-check-wifexited-after-wait-in-order-to-kill-child-processes-in-lin
      //Source for WEXITSTATUS: https://stackoverflow.com/questions/20465039/what-does-wexitstatusstatus-return
      if(WIFEXITED(finishedChildProcesses)){
      printf("Child process %d terminated normally with exit status %d\n", child, WEXITSTATUS(finishedChildProcesses)); 
      } else {
        printf("Child process terminated abnormally");
      }
      
    }
    
  }
  /* print the array A if the macro TEST is defined */
#ifdef TEST
  printf("A = {");
  for(int i=0; i<N-1; i++){
    printf("%d, ", A[i]);
  }
  printf("%d}\n", A[N-1]);
#endif

  /* print the final average */
  int finalAverage = (avg/P);
  fprintf(stderr, "Final Average = %d\n", finalAverage); //final average calculations

  exit(0);
}

/*
everything OUTSIDE of main() goes here:
*/

/* Definition of the function initialize */
void initialize(int M[N]){
    int i;
    srand(time(NULL));
    for(i=0; i<N; i++){
        M[i] = rand() % N;
    }
}
/* Define the Handler for SIGUSR1 here */
void sigusr1_handler(int sig, siginfo_t *value, void *ucontext){
  /* Follow the guidelines to write safe signal handlers*/
  int sum = value->si_value.sival_int; //from project description
  avg = avg + sum; 
  average++; 
  fprintf(stderr, "Parent process caught SIGUSR1 with partial average: %d\n", sum);

  //is it possible to void out ucontext and sig?

}

/* Define the Wrapper functions for the system calls */
// wrapper functions will be here: 
void unix_error(const char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(0);
}

//need unix_error 

pid_t Fork(){ //used to create a new child process
  pid_t pid;
  if((pid=fork()) < 0){
    unix_error("fork error:");
  }
  return pid;
}

pid_t Wait(int *status) {
  pid_t pid;
  if ((pid = wait(status)) < 0) {
    unix_error("Wait error");
  }
  return pid;
}

//suspends execution
pid_t Waitpid(pid_t pid, int *status, int options) {
  pid_t pid2; //different name for pid_t as pid is an argument
  if ((pid2 = waitpid(pid, status, options)) < 0) {
    unix_error("Waitpid error");
  }
  return pid2;
}

//used to send signal
int Sigqueue(pid_t pid, int signum, union sigval value){
  int result;
  if ((result = sigqueue(pid, signum, value)) < 0) {
    unix_error("Sigqueue error");
  }
  return result;
}


int Sigemptyset(sigset_t *set) {
  int result;
  // clear the set of any signals for the new signals
  if ((result = sigemptyset(set)) < 0) {
    unix_error("Sigemptyset error");
  }
  return result;
}

int Sigfillset(sigset_t *set) {
  int result;
  //first, result will hold the set of all signals
  if ((result = sigfillset(set)) < 0) {
    unix_error("Sigfillset error");
  }
  return result;
}

int Sigaction(int signum, const struct sigaction *new_act, struct sigaction *old_act) {
  int result;
  // sets up the signal action
  if ((result = sigaction(signum, new_act, old_act)) < 0) {
    unix_error("Sigaction error");
  }
  return result;
}

int Sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
  int result;
  // this can change the signal mask
  if ((result = sigprocmask(how, set, oldset)) < 0) {
    unix_error("Sigprocmask error");
  }
  return result;
}

ssize_t Write(int d, const void *buffer, size_t nbytes) {
  ssize_t result;
  if ((result = write(d, buffer, nbytes)) < 0) {
    unix_error("Write error");
  }
  return result;
}

typedef void handler_t;

// referenced partially from lecture 11, slide 55
handler_t *Signal(int signum, handler_t *handler){
  struct sigaction action, old_action; // this is for the current and old signal actions
  action.sa_handler = handler; //handler function
  Sigemptyset(&action.sa_mask); //clear signal for a new signal
  action.sa_flags = SA_SIGINFO | SA_RESTART; // this is one change and not only just SA_RESTART
  //additional flags
  if(Sigaction(signum, &action, &old_action) < 0){
    unix_error("Signal Error");
  }

  return (old_action.sa_handler);

}

