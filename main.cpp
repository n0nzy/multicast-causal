#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int vector_clock[4] = {0, 0, 0, 0}; // Initialize vector clock

void error(const char* msg) {
    perror(msg);
    exit(1);
}

int print_clockstate(int process_id) {
    cout << "[ ";
    for (int i; i < 4; ++i) {
        cout << vector_clock[i] << ", ";
    }
    cout << "]";
    return 0;
}

void* start_server (void* data) {

}

void* start_client (void* data) {

}

int main(int argc, char *argv[]) {
    std::cout << "Hello, World!" << std::endl;

    if (argc != 2) {
        error("\nProgram started with wrong parameters \n\tHow To Use: program processID\n");
    }

    int process_id = atoi(argv[1]);
    vector_clock[process_id-1] = vector_clock[process_id -1] + 1;
    //std::cout << "I am process: " << process_id << " and my clock is:" << print_clockstate(process_id) << std::endl;
    std::cout << "I am process: " << process_id << " and my clock is: ";
    print_clockstate(process_id); cout << "\n";

    pthread_t tid_server, tid_client;
    if (pthread_create(&tid_server, NULL, start_server, NULL)) {
        error("Error in spanning server thread");
    }
    if (pthread_create(&tid_client, NULL, start_client, NULL)) {
        error("Error in spanning client thread");
        exit(-1);
    }
    pthread_join(tid_server, NULL);
    pthread_join(tid_client, NULL);

    return 0;
}