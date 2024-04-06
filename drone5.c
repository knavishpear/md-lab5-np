//THINGS TO DO//
/*
if TTL == 0 , throw message away / ignore
if TTL != 0, decrement ttl, change location, forward to all
need to add location to send out with msg, probably collect this from config file
may also need to add other things from msg protocol, might be able to ignore some of them
Also need to forward msg after

*/
//CSCI 3762 NETWORK PROGRAMMING
//MILES DIXON
//LAB5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#include <sys/select.h>

#define MAX_LOCATIONS 100
#define BUFFER_SIZE 1024

struct ServerConfig {
    char serverIP[16];
    int port;
    int location;
};

struct Message {
    char msg[BUFFER_SIZE];
    int toPort;
    int fromPort;
};

struct _tokens {
    char key[100];
    char value[100];
};

// Function to find tokens in the message
int findTokens(char *buffer, struct _tokens *tokens) {
    int counter = 0; // Number of tokens found
    char *ptr;

    // Tokenize first on the space ' '
    ptr = strtok(buffer, " ");
    while (ptr != NULL) {
        memset(tokens[counter].key, 0, 100);
        memset(tokens[counter].value, 0, 100);

        int i = 0; // Index for iterating over ptr
        int flag = 0; // Flag to indicate key or value portion
        int k = 0; // Index for storing value in tokens[counter].value
        for (i = 0; i < strlen(ptr); i++) {
            if (flag == 0) { // Doing the key portion
                if (ptr[i] != ':')
                    tokens[counter].key[i] = ptr[i];
                else // Found a ':', move to value portion
                    flag = 1;
            } else { // Doing the value portion
                if (ptr[i] == '^') { // Undoing the cleansing of msg portion
                    ptr[i] = ' ';
                }
                tokens[counter].value[k] = ptr[i];
                k++;
            }
        }
        ptr = strtok(NULL, " ");
        counter++;
    }
    return counter;
}

//parse the config file and info from config file
int parseConfigFile(const char *filename, struct ServerConfig *servers, int *num_servers) {
    FILE *config_file = fopen(filename, "r");
    if (config_file == NULL) {
        perror("Error opening config file");
        return -1;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), config_file) != NULL) {
        if (*num_servers >= MAX_LOCATIONS) {
            fprintf(stderr, "Maximum number of servers reached.\n");
            break;
        }
        sscanf(line, "%s %d %d", servers[*num_servers].serverIP, &servers[*num_servers].port, &servers[*num_servers].location);
        (*num_servers)++;
    }

    fclose(config_file);
    return 0;
}

//euclidean distance calculator
double distance(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    return sqrt(dx * dx + dy * dy);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <config_file> <port> <gridsize>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[2]);
    int gridSize = atoi(argv[3]);
    if (port <= 0 || port > 65535 || gridSize <= 0) {
        fprintf(stderr, "Invalid port number or grid size.\n");
        exit(1);
    }

    struct ServerConfig servers[MAX_LOCATIONS];
    int num_servers = 0;

    //parse config and store info
    if (parseConfigFile(argv[1], servers, &num_servers) != 0) {
        exit(1);
    }

    //create / bind socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    //get selected port
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(sockfd, (struct sockaddr *)&addr, &len);

    printf("Server listening on port %d...\n", ntohs(addr.sin_port));

    char buffer[BUFFER_SIZE];
    struct Message userMessage;

    fd_set read_fds;
    while (1) {
        // Display "Enter a message" prompt
        printf("Enter a message: \n");
        fflush(stdout); // Force immediate output

        // Reset read_fds and add file descriptors
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);

        // Wait for activity on either standard input or socket
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, NULL);
        if (activity == -1) {
            perror("select");
            exit(1);
        }

// Handle standard input
if (FD_ISSET(STDIN_FILENO, &read_fds)) {
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        // Send message to all servers
        for (int i = 0; i < num_servers; ++i) {
            struct sockaddr_in destAddr;
            memset(&destAddr, 0, sizeof(destAddr));
            destAddr.sin_family = AF_INET;
            destAddr.sin_addr.s_addr = inet_addr(servers[i].serverIP);
            destAddr.sin_port = htons(servers[i].port);
	    //printf("Buffer: %s\n", buffer);

	    char testBuff[BUFFER_SIZE+50];
	    int locTest = -1;
	    //repeat of function below, change this later to make cleaner
	    for(int i=0; i<num_servers; ++i){
		if(servers[i].port == port){
		    locTest = servers[i].location;
		}
	    }
	    //printf("loctest: %d\n", locTest);
	    char additions[50] = " location:";
////////////////////////FIX HERE^^^^^^^^
            ssize_t bytesSent = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
            if (bytesSent == -1) {
                perror("sendto");
                continue;
            }
            printf("Sent message to Location %d (IP: %s, Port: %d)\n", servers[i].location, servers[i].serverIP, servers[i].port);
        }
    }
}




// Handle incoming messages
if (FD_ISSET(sockfd, &read_fds)) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    ssize_t bytesRead = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientLen);
    if (bytesRead == -1) {
        perror("recvfrom");
        continue;
    }

    // Print the received buffer
    printf("Received Buffer: %s", buffer);

    buffer[bytesRead] = '\0'; // Null terminate the received message

    // Check if the message is intended for this server
    int receivedToPort = -1;
    int receivedFromPort = -1;
    int receivedTTL = -1;
    char receivedMsg[BUFFER_SIZE];

    struct _tokens tokens[4];
    int num_tokens = findTokens(buffer, tokens);

    for (int i = 0; i < num_tokens; ++i) {
        if (strcmp(tokens[i].key, "msg") == 0) {
            strncpy(receivedMsg, tokens[i].value, BUFFER_SIZE);
        } else if (strcmp(tokens[i].key, "toPort") == 0) {
            receivedToPort = atoi(tokens[i].value);
        } else if (strcmp(tokens[i].key, "fromPort") == 0) {
            receivedFromPort = atoi(tokens[i].value);
        } else if (strcmp(tokens[i].key, "TTL") == 0){
	    receivedTTL = atoi(tokens[i].value);
	}
    }

    if (receivedToPort != port) {
        printf("Received message not for me\n");

	//NOW FORWARD MSG TO ALL w/ FORLOOP and DECREMENT TTL, also need to add location
	
    } else {
        // Further processing of the received message
        //printf("Received message: %s\n", receivedMsg);

        int senderLocation = -1;
        for (int i = 0; i < num_servers; ++i) {
            if (servers[i].port == receivedFromPort) {
                senderLocation = servers[i].location;
                break;
            }
        }

        int myLocation = -1;
        for (int i = 0; i < num_servers; ++i) {
            if (servers[i].port == port) {
                myLocation = servers[i].location;
                break;
            }
        }
        // Check if the message is within range or out of range
        // Assuming some range checking logic here
	int senderX = (senderLocation - 1) % gridSize;
        int senderY = (senderLocation - 1) / gridSize;
        int myX = (myLocation - 1) % gridSize;
        int myY = (myLocation - 1) / gridSize;
        double dist = distance(myX, myY, senderX, senderY);
	//printf("Gridsize: %d  dist: %lf\n", gridSize, dist);
	printf("ReceivedTTL: %d\n", receivedTTL);
        if (dist <= 2) {
	    if(receivedTTL > 0){
            	printf("Received message: msg: %s  toPort: %d  fromPort: %d  TTL: %d\n", receivedMsg, receivedToPort, receivedFromPort, receivedTTL);
	    } else{
		printf("Received message w/ TTL expired: %d\n", receivedTTL);
	    }
	} else {
            printf("Received message out of range\n");
        }
    }
}


    }//endwhile

    close(sockfd);
    return 0;
}
