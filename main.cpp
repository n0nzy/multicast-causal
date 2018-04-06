#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_MSG_SIZE 1024
char sendBuffer[MAX_MSG_SIZE];
char recvBuffer[MAX_MSG_SIZE];

using namespace std;

int vector_clock[4] = {0, 0, 0, 0}; // Initialize vector clock
uint64_t process_id = 0;

struct process_params {
    uint64_t id;
    uint64_t port;
};

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

/*
 * Set the server's parameters such as host address and port number.
 */
struct sockaddr_in setup_server_params(const char *hostname, uint16_t port) {
    struct sockaddr_in serverParams;
    memset(&serverParams, '\0', sizeof(struct sockaddr_in));

    serverParams.sin_family = AF_INET;
    serverParams.sin_addr.s_addr = htonl(INADDR_ANY);
    serverParams.sin_port = htons(port);

    return serverParams;
}

int socket_setup() {
    const int TRUE = 1;
    int sockfd = -1; int opt = TRUE;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("\n Error : Could not create socket \n");
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        error("setsockopt");
    }
    return sockfd;
}

int socket_bind(int sockfd, struct sockaddr_in serv_addr) {

    int recvValue = bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (recvValue < 0) {
        error("Error in binding");
    }

    return recvValue;  // a value < 0 is a failure
}

/*
 * Here we check the condition for Rule 2, we need to check all other values of the vector clock contained in the received message from the sender
 * */
bool cond_others(int M[], int VC[], int sender_id, int array_size) {
    bool result = true; // we'll give sender's VC benefit of the doubt, i.e. we'll assume it satisfies causality.
    for (int i=0; i < array_size; i++) {
        if (i != sender_id) {  // skip check for the sender's entry in the vector clock.
            cout << "i: " << i << " " << endl;
            if (M[i] > VC[i]) { // if any of sender's VC values is NOT 'equal to/less than' host process's VC values, rule is broken, set to false.
                result = false;
                cout << "Rule is broken! will have to buffer!" << endl;
            }
        }
    }
    return result;
}

int server_socket_connect(int sockfd, struct sockaddr_in serv_addr, int port) {

    printf("\t::Waiting for New Request data\n");

    /* The accept call blocks until a connection is found */
    int connfd = accept(sockfd, (struct sockaddr*) NULL, NULL);

    int numBytes = read(connfd,recvBuffer,sizeof(recvBuffer)-1); // This is where server gets input from client
    if (numBytes < 0) error("ERROR reading from socket");
    //printf("Data Received from client: %s \n",recvBuffer); // This is displayed on the server end.
    cout << "Data received from client:  " << recvBuffer << endl;

    /* --------------------------------------------------------------- */
    char* sender = strtok(recvBuffer, " "); // parse request data, First Level extraction, get the sending process's ID,
    cout << "process ID: " << sender << endl;
    int sender_id = atoi(sender) - 1;  // transform into array index format

    // msg[] is an array to store the sender's vector clock values
    int msg[100]; int indexx = 0;
    char* senders_vclock = strtok(NULL, "-"); // Second Level extraction, get Vector clock array values
    while (senders_vclock != NULL) {
        //cout << senders_vclock << endl;
        cout << "senders vclk: " << senders_vclock << " | indexx: " << indexx << endl;
        //msg[indexx] = indexx;
        msg[indexx] = atoi(senders_vclock);
        senders_vclock = strtok(NULL, "-"); // point to next item in recvBuffer
        indexx++;
    }
    cout << "end of parsing" << endl;
    /* --------------------------------------------------------------- */

    if (msg[sender_id] == vector_clock[sender_id] + 1 && (cond_others(msg, vector_clock, sender_id, 4)) ) {
        vector_clock[sender_id] = msg[sender_id];
        cout << "send to application.." << endl;
    }
    else {
        cout << "buffering.." << endl;
    }

    return 1;
}

void* start_server (void* parameters) {
    process_params* params = (process_params*) parameters;
    uint64_t process_id = params->id;
    uint64_t port = params->port;

    const int CONN_BACKLOG_NUM = 5;
    //int port = 8001;

    /* Initiate Server Parameters */
    struct sockaddr_in myAddr = setup_server_params("127.0.0.1", port);

    while (1) {
        int sockfd = socket_setup();

        //inform user of socket number - used in send and receive commands
        printf("\nNew connection, socket fd is %d , ip is : %s , port : %d \n" , sockfd , inet_ntoa(myAddr.sin_addr) , ntohs(myAddr.sin_port));

        socket_bind(sockfd, myAddr);

        listen(sockfd, CONN_BACKLOG_NUM);

        //printf("listening on sockfd: %lu on port: %d \n", sockfd, port);

        server_socket_connect(sockfd, myAddr, port); // pass a ref to the processes clock to update later.
        close(sockfd);
    }
}

int client_connect2server(int sockfd, struct sockaddr_in serv_addr, int port) {

    memset(&serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        error("\n inet_pton error occured\n");
    }

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("\n Error : Connect Failed \n");
    }

    return sockfd;
}

void* start_client (void* data) {
    int sendto = 0;
    cout << "I am the client" << endl;
    cout << "what process do you want send a msg to? " << endl;
    cin >> sendto;
    cout << "you have indicated you want to send a msg to process: " << sendto << endl;

    /* Preparing parameters to send request to server. */
    struct sockaddr_in serverAddress = setup_server_params("127.0.0.1", sendto);
    int sockfd = socket_setup();

    // TODO : (optional) Given a Port #, look up the corresponding process
    printf("Preparing request to Process %d with sockfd %d on port %d\n", 1, sockfd, sendto);
    sockfd = client_connect2server(sockfd, serverAddress, sendto);

    printf("Sending Request...\n");  // send msg to process
    memset(sendBuffer, '\0',sizeof(sendBuffer));

    sprintf(sendBuffer, "%lu", process_id);  // value to be sent
    //strcat(sendBuffer, " 1-0-0-0");
    /* Now pass vector clock contents to server */
    int indexx = 0; char temp[128];
    string vcstring = "";
    for (int i = 0; i < 4; ++i) {
        vcstring.push_back('0'+vector_clock[i]);
        if (i != 4-1) {
            vcstring.push_back('-');
        }
        //indexx = indexx + sprintf(&temp[indexx], "%d", vector_clock[i]);
    }
    char* cstring = new char[vcstring.length()+1]; std::strcpy(cstring, vcstring.c_str());

    strcat(sendBuffer, " ");
    strcat(sendBuffer, cstring);

    int numBytes = write(sockfd, sendBuffer, strlen(sendBuffer));  // sending operation
    if (numBytes < 0)
        error("Error sending request.");
    printf("Request: sent!\n");

}

int main(int argc, char *argv[]) {
    std::cout << "Hello, World!" << std::endl;

    if (argc != 3) {
        error("\nProgram started with wrong parameters \n\tHow To Use: program port processID\n");
    }

    //uint64_t process_id = atoi(argv[2]);
    process_id = atoi(argv[2]);

    struct process_params input_params;
    input_params.port = atoi(argv[1]);
    input_params.id = process_id;


    vector_clock[process_id-1] = vector_clock[process_id -1] + 1;
    std::cout << "I am process: " << process_id << " and my clock is: ";
    print_clockstate(process_id); cout << "\n";

    pthread_t tid_server, tid_client;

    if (pthread_create(&tid_server, NULL, start_server, (void*) &input_params)) {
        error("Error in spawning server thread");
    }

    if (pthread_create(&tid_client, NULL, start_client, NULL)) {
        error("Error in spawning client thread");
        exit(-1);
    }
    pthread_join(tid_server, NULL);
    pthread_join(tid_client, NULL);

    return 0;
}