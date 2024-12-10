#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    int BankAccount;    // Shared bank account balance
    sem_t mutex;        // Semaphore for mutual exclusion
} SharedData;

void DearOldDad(SharedData *ShmPTR);
void PoorStudent(SharedData *ShmPTR);

int main() {
    int ShmID;
    pid_t pid;

    // Seed random number generator
    srand(time(NULL));

    // Create shared memory segment
    ShmID = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }
    printf("Server has received shared memory for BankAccount and semaphores...\n");

    // Attach shared memory
    SharedData *ShmPTR = (SharedData *) shmat(ShmID, NULL, 0);
    if ((void *) ShmPTR == (void *) -1) {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    printf("Server has attached the shared memory...\n");

    // Initialize shared data
    ShmPTR->BankAccount = 100;  // Initial BankAccount value
    sem_init(&(ShmPTR->mutex), 1, 1);  // Initialize semaphore for mutual exclusion

    // Fork child process
    pid = fork();
    if (pid < 0) {
        printf("*** fork error (server) ***\n");
        exit(1);
    } else if (pid == 0) {
        // Child process
        PoorStudent(ShmPTR);
        exit(0);
    } else {
        // Parent process
        DearOldDad(ShmPTR);
    }

    // Wait for child process to complete
    wait(NULL);

    // Cleanup shared memory and semaphore
    sem_destroy(&(ShmPTR->mutex));  // Destroy semaphore
    shmdt((void *) ShmPTR);  // Detach shared memory
    shmctl(ShmID, IPC_RMID, NULL);  // Remove shared memory

    printf("Server has cleaned up shared memory and semaphores...\n");
    printf("Server exits...\n");
    return 0;
}

void DearOldDad(SharedData *ShmPTR) {
    while (1) {
        sleep(rand() % 6);  // Sleep 0-5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(&(ShmPTR->mutex));  // Lock semaphore
        int localBalance = ShmPTR->BankAccount;
        if (rand() % 2 == 0) {  // Even number
            if (localBalance < 100) {
                int deposit = rand() % 101;  // Random deposit 0-100
                ShmPTR->BankAccount += deposit;
                printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", deposit, ShmPTR->BankAccount);
            } else {
                printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(&(ShmPTR->mutex));  // Unlock semaphore
    }
}

void PoorStudent(SharedData *ShmPTR) {
    while (1) {
        sleep(rand() % 6);  // Sleep 0-5 seconds
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(&(ShmPTR->mutex));  // Lock semaphore
        int localBalance = ShmPTR->BankAccount;
        if (rand() % 2 == 0) {  // Even number
            int need = rand() % 51;  // Random need 0-50
            printf("Poor Student needs $%d\n", need);
            if (need <= localBalance) {
                ShmPTR->BankAccount -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, ShmPTR->BankAccount);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(&(ShmPTR->mutex));  // Unlock semaphore
    }
}