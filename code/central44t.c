#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_PROCESSES 4
#define FIRST_NUM_PROCESSES 4
#define SECOND_NUM_PROCESSES 8
#define FIRST_PROCESS 1
#define SECOND_PROCESS 5
sem_t mutex;
struct {
  long priority;        //message priority
  int temp;             //temperature
  int pid;              //process id
  int stable;           //boolean for temperature stability
} msgp, cmbox;

struct {
  long priority;        //message priority
  int temp;             //temperature
  int pid;              //process id
  int stable;           //boolean for temperature stability
} msgpTwo, cmboxTwo;

struct processInfo{

  int mailbox;
  int initTemp;
  int process_Num;
  int p_Range;
  int counter;
  int group_No;
};


void  *process(void * arg) {
  sem_wait(&mutex);
    struct processInfo *p = (struct processInfo*)arg;
  //Set up local variables
    //counter for loops
   
    int i,
      result,
      length, 
      status;             
    //central process ID
    int uid = 0;                               
    //mailbox IDs for all processes
    int msqid[NUM_PROCESSES];       
    //boolean to denote temp stability
    int unstable = 1;  
    //array of process temperatures        
    int tempAry[NUM_PROCESSES];   
    
    //Create the Central Servers Mailbox
    int msqidC = msgget(p->mailbox, 0600 | IPC_CREAT);
    
    

      for(i = p->process_Num; i <= p->p_Range; i++){  
          msqid[(p->counter-1)] = msgget((p->mailbox + i), 0600 | IPC_CREAT); 
          p->counter++;
      }

    
       
    
    //Initialize the message to be sent
    msgp.priority = 1;
    msgp.pid = uid;
    msgp.temp = p->initTemp;
    msgp.stable = 1;

    /* The length is essentially the size of the structure minus sizeof(mtype) */
    length = sizeof(msgp) - sizeof(long);

    //While the processes have different temperatures
    while(unstable == 1){
          
         
          int sumTemp = 0;        //sum up the temps as we loop
          int stable = 1;            //stability trap
        
            // Get new messages from the processes
            for(i = 0; i < NUM_PROCESSES; i++){
                result = msgrcv( msqidC, &cmbox, length, 1, 0);
                
                /* If any of the new temps are different from the old temps then we are still unstable. Set the  new temp to the corresponding process ID in the array */
                if(tempAry[(cmbox.pid - 1)] != cmbox.temp) {
                      stable = 0;
                      tempAry[(cmbox.pid - 1)] = cmbox.temp;
                }

                //Add up all the temps as we go for the temperature algorithm
                sumTemp += cmbox.temp;
            }

         
          /*When all the processes have the same temp twice: 1) Break the loop 2) Set the messages stable field  to stable*/
          if(stable){
              
              unstable = 0;
              msgp.stable = 0;
          }
          else { //Calculate a new temp and set the temp field in the message
              
              int newTemp = (msgp.temp + 1000*sumTemp) / (1000*NUM_PROCESSES + 1);
              usleep(100000);
          msgp.temp = newTemp;
          printf("The new temp in GROUP %d is:  %d\n",p->group_No,newTemp);
          }
         
          /* Send a new message to all processes to inform of new temp or stability */
        for(i = 0; i < NUM_PROCESSES; i++){
              result = msgsnd( msqid[i], &msgp, length, 0);
              
        } 
    }

    //Remove the mailbox
 
    status = msgctl(msqidC, IPC_RMID, 0);
 
    //Validate nothing when wrong when trying to remove mailbox
     if(status != 0){
          printf("\nERROR closing mailbox\n");
    }
          /* up semaphore */
    sem_post(&mutex);
    pthread_exit(NULL);

}


int main(int argc, char *argv[]) {

  struct timeval t1, t2;
      double elapsedTime;
    // start timer
    gettimeofday(&t1, NULL);
    int i = 0;
    printf("\nStarting Server...\n");
    sem_init(&mutex, 0, 1); 
  
  //Validate that a temperature was given via the command line
    if(argc != 5) {
        printf("USAGE: Too few arguments --./central.out Temp");
        exit(0);
    }
    
    struct processInfo process1, process2;

    process1.mailbox = atoi(argv[3]);
    process1.initTemp = atoi(argv[1]);
    process1.process_Num = FIRST_PROCESS;
    process1.p_Range = FIRST_NUM_PROCESSES;
    process1.counter = 1;
    process1.group_No = 1;

    process2.mailbox = atoi(argv[4]);
    process2.initTemp = atoi(argv[2]);
    process2.process_Num = SECOND_PROCESS;
    process2.p_Range = SECOND_NUM_PROCESSES;
    process2.counter = 1;
    process2.group_No = 2;


    pthread_t thread[2]; 
    
    pthread_create(&thread[0], NULL, &process, &process1);
    pthread_create(&thread[1], NULL, &process, &process2);
        
    for(i = 0; i < 2; i++){
      pthread_join(thread[i], NULL);
    }
  // if(pid == 0){
    
  //   //child process
  //   initTemp = atoi(argv[2]);
  //   mailbox = atoi(argv[4]);
  //   group2 = process(initTemp, mailbox, SECOND_PROCESS, SECOND_NUM_PROCESSES,1,2);
  //   printf("Temperature Stabilized in Group2: %d\n", group2);
  //   exit(0);
  // }
  // else if(pid > 0){
  //   //parent process
  //   // wait(NULL);
  //   initTemp = atoi(argv[1]);
  //   mailbox = atoi(argv[3]);
  //   wait(NULL);
  //   group1 = process(initTemp, mailbox, FIRST_PROCESS, FIRST_NUM_PROCESSES,1,1);
  // }
  // else{
    
  //   printf("Fork failed\n" );
  // }

  printf("Temperature Stabilized in Group1: \n");
 
  // stop timer
    gettimeofday(&t2, NULL);

      // compute and print the elapsed time in millisec
      elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
      elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
  printf("The elapsed time is %fms\n", elapsedTime);
  sem_destroy(&mutex); /* destroy semaphore */


return EXIT_SUCCESS;

}

