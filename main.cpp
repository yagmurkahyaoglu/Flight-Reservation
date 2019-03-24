#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <unistd.h>

using namespace std;

pthread_mutex_t mutexC = PTHREAD_MUTEX_INITIALIZER;         // mutex that controls critical sections for client threads
pthread_mutex_t mutexS = PTHREAD_MUTEX_INITIALIZER;         // mutex that controls critical sections for server threads
pthread_mutex_t mutexWrite = PTHREAD_MUTEX_INITIALIZER;     // mutex that controls critical sections for writing
int* seats = NULL;

struct client {     // struct to hold information of a client
    int totalSeats;
    int id;
    int seat = 0;
    bool isSeated = false;
    bool wait = true;
};

void* clientMethod (void* arg){

    usleep(rand()% 151 + 50);       // sleep the thread for a random time between 50-200 ms
    struct client *clientInfo = (struct client*) arg;

    while(!clientInfo->isSeated) { // try random seat numbers until finding an available seat
        pthread_mutex_lock(&mutexC);    // lock to prevent preempting during assignments
        int randomSeat = rand() % clientInfo->totalSeats + 1;
        clientInfo->seat = randomSeat;
        clientInfo->wait = true;
        pthread_mutex_unlock(&mutexC);
        while(clientInfo->wait && !clientInfo->isSeated) {} // busy wait until server responds
    }
    pthread_mutex_lock(&mutexWrite);    // lock to prevent preempting while writing
    cout << "Client" << clientInfo->id << " reserves Seat" << clientInfo->seat << endl;
    pthread_mutex_unlock(&mutexWrite);
    pthread_exit(0);
}

void* serverMethod (void* arg){

    struct client *clientInfo = (struct client*) arg;

    while (clientInfo->seat == 0) {}    // busy wait until client sends a request

    while(!clientInfo->isSeated){       // repeat until client finds an available seat
        pthread_mutex_lock(&mutexS);    // lock to prevent preempting
        if(seats[clientInfo->seat-1] == 0){     // if the seat is empty assign it to the client
            seats[clientInfo->seat-1] = 1;
            clientInfo->isSeated = true;
        }
        else {
            clientInfo->wait = false;
            while(!clientInfo->wait) {} // busy wait until client sends another request
        }
        pthread_mutex_unlock(&mutexS);
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {

    int N = stoi(argv[1]);    // number of seats
    //int N = 10;

    struct client info[N];      // array of client structs for each client thread

    pthread_t clients[N];       // array of client threads
    pthread_t servers[N];       // array of server threads

    seats = (int *)malloc(sizeof(int) * N);     // array to control each seat

    freopen("output.txt", "w", stdout);     // open output.txt to write
    cout << "Number of total seats: " << N << endl;

    for(int k = 0; k < N; k++){     // initialize all seats to empty
        seats[k] = 0;
    }

    for(int i = 0; i < N; i++){
        info[i].id = i+1;
        info[i].totalSeats = N;

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        pthread_create(&clients[i], &attr, clientMethod, &info[i]);     // create client threads
        pthread_create(&servers[i], &attr, serverMethod, &info[i]);     // create server threads
    }

    for(int j = 0; j < N; j++){             // join all threads
        pthread_join(clients[j], NULL);
        pthread_join(servers[j], NULL);
    }

    cout << "All seats are reserved." << endl;
    free(seats);
    fclose(stdout);

    return 0;
}