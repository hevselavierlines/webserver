#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <iostream>

#define PORT "8080"
#define FOLDER "/Users/Afaci/Documents/FHBachelor/iOS/jonas/webservertest/webservertest"

// structure to hold the return code and the filepath to serve to client.
typedef struct {
    int returncode;
    char *filename;
} httpRequest;

// send a message to a socket file descripter
long sendMessage(int fd, char *msg) {
    if(fd > 0) {
        return write(fd, msg, strlen(msg));
    } else {
        return 0;
    }
}

// headers to send to clients
char *header200 = "HTTP/1.0 200 OK\nServer: JOSEF v0.1\nContent-Type: text/html\n\n";
char *header400 = "HTTP/1.0 400 Bad Request\nServer: JOSEF v0.1\nContent-Type: text/html\n\n";
char *header404 = "HTTP/1.0 404 Not Found\nServer: JOSEF v0.1\nContent-Type: text/html\n\n";

void printHeader(int fd, int returncode) {
    // Print the header based on the return code
    switch (returncode)
    {
        case 200:
            sendMessage(fd, header200);
            break;
            
        case 400:
            sendMessage(fd, header400);
            break;
            
        case 404:
            sendMessage(fd, header404);
            break;
            
    }
}


/* 
 * Prints the file to the client and returns the total amount of sent bytes.
 */
long long printFile(int fd, char *filename) {
    
    /* Open the file filename and echo the contents from it to the file descriptor fd */
    
    // Attempt to open the file
    FILE *read;
    if( (read = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file in printFile()\n");
        exit(EXIT_FAILURE);
    }
    
    // Get the size of this file for printing out later on
    long long totalsize;
    struct stat st;
    stat(filename, &st);
    totalsize = st.st_size;
    
    // Variable for getline to write the size of the line its currently printing to
    size_t size = 1;
    
    // Get some space to store each line of the file in temporarily
    char *temp;
    if(  (temp = (char*)malloc(sizeof(char) * size)) == NULL )
    {
        fprintf(stderr, "Error allocating memory to temp in printFile()\n");
        exit(EXIT_FAILURE);
    }
    
    
    // Int to keep track of what getline returns
    long end;
    
    // While getline is still getting data
    while( (end = getline( &temp, &size, read)) > 0)
    {
        sendMessage(fd, temp);
    }
    
    // Final new line to signalise the end
    sendMessage(fd, "\n");
    
    // Free temp as we no longer need it
    free(temp);
    
    // Return how big the file we sent out was
    return totalsize;
    
}

/*
 * get a message from the socket until a blank line is recieved.
 * The blank line means that the client finished sending his data.
*/
char *getMessage(int fd) {
    
    // A file stream
    FILE *sstream;
    
    // Try to open the socket to the file stream and handle any failures
    if( (sstream = fdopen(fd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file descriptor in getMessage()\n");
        exit(EXIT_FAILURE);
    }
    
    // Size variable for passing to getline
    size_t size = 1;
    
    char *block;
    
    // Allocate some memory for block and check it went ok
    if( (block = (char*)malloc(sizeof(char) * size)) == NULL )
    {
        fprintf(stderr, "Error allocating memory to block in getMessage\n");
        exit(EXIT_FAILURE);
    }
    
    // Set block to null
    *block = '\0';
    
    // Allocate some memory for tmp and check it went ok
    char *tmp;
    if( (tmp = (char*)malloc(sizeof(char) * size)) == NULL )
    {
        fprintf(stderr, "Error allocating memory to tmp in getMessage\n");
        exit(EXIT_FAILURE);
    }
    // Set tmp to null
    *tmp = '\0';
    
    // Long to keep track of what getline returns
    long end;
    // Int to help use resize block
    int oldsize = 1;
    
    // While getline is still getting data
    while( (end = getline( &tmp, &size, sstream)) > 0)
    {
        // If the line its read is a caridge return and a new line were at the end of the header so break
        if( strcmp(tmp, "\r\n") == 0)
        {
            break;
        }
        
        // Resize block
        block = (char*)realloc(block, size+oldsize);
        // Set the value of oldsize to the current size of block
        oldsize += size;
        // Append the latest line we got to block
        strcat(block, tmp);
    }
    
    // Free tmp a we no longer need it
    free(tmp);
    
    // Return the header
    return block;
    
}

// parse a HTTP request and return an object with return code and filename
httpRequest parseRequest(char *msg){
    httpRequest ret;
    
    // Variable to store the filename in
    char * filename;
    // Allocate some memory for the filename and check it went OK
    if( (filename = new char[strlen(msg)]) == NULL)
    {
        fprintf(stderr, "Error allocating memory to file in getFileName()\n");
        exit(EXIT_FAILURE);
    }
    
    // Get the filename from the header
    sscanf(msg, "GET %s HTTP/1.1", filename);
    std::cout << "GET " << filename << "\n\n";
    
    // Check if its a directory traversal attack
    char *badstring = "..";
    char *test = strstr(filename, badstring);
    
    char *path = new char[strlen(FOLDER) + strlen(filename) + 1];
    strcat(path, FOLDER);
    strcat(path, filename);
    
    // Check if they asked for / and give them index.html
    int test2 = strcmp(filename, "/");
    
    // Check if the page they want exists
    FILE *exists = fopen(path, "r" );
    
    // If the badstring is found in the filename
    if( test != NULL )
    {
        // Return a 400 header and 400.html
        ret.returncode = 400;
        ret.filename = "400.html";
    }
    
    // If they asked for / return index.html
    else if(test2 == 0)
    {
        ret.returncode = 200;
        char *defaultPath = new char[256];
        strcat(defaultPath, FOLDER);
        strcat(defaultPath, "/index.html");
        ret.filename = defaultPath;
    }
    
    // If they asked for a specific page and it exists because we opened it sucessfully return it
    else if( exists != NULL )
    {
        
        ret.returncode = 200;
        ret.filename = path;
        // Close the file stream
        fclose(exists);
    }
    
    // If we get here the file they want doesn't exist so return a 404
    else
    {
        ret.returncode = 404;
        char *errorPage = new char[256];
        strcat(errorPage, FOLDER);
        strcat(errorPage, "/404.html");
        ret.filename = errorPage;
    }
    
    // Return the structure containing the details
    return ret;
}

/**
 * This is the thread routine.
 * Firstly receiving the data from the client.
 * Secondly respond with the data in the server file system.
 */
void *serveClient(void *commands) {
    int new_conn_fd = *(int*)commands;
    char* header = getMessage(new_conn_fd);
    // Parse the request
    httpRequest details = parseRequest(header);
    free(header);
    // Write the response to the user
    printHeader(new_conn_fd, details.returncode);
    printFile(new_conn_fd, details.filename);
    close(new_conn_fd);
    
    return NULL;
}

/**
 * The main routine
 * Firstly setting up the socket.
 * Secondly waiting for clients.
 * If a client connects start a thread for them.
 */
int main(int argc, char * argv[])
{
    int status;
    struct addrinfo hints, * res;
    int listener;
    
    
    // Before using hint you have to make sure that the data structure is empty
    memset(& hints, 0, sizeof hints);
    // Set the attribute for hint
    hints.ai_family = AF_UNSPEC; // We don't care V4 AF_INET or 6 AF_INET6
    hints.ai_socktype = SOCK_STREAM; // TCP Socket SOCK_DGRAM
    hints.ai_flags = AI_PASSIVE;
    
    // Fill the res data structure and make sure that the results make sense.
    status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr,"getaddrinfo error: %s\n",gai_strerror(status));
    }
    
    // Create Socket and check if error occured afterwards
    listener = socket(res->ai_family,res->ai_socktype, res->ai_protocol);
    if (listener < 0) {
        fprintf(stderr,"socket error: %s\n",gai_strerror(status));
    }
    
    int set = 1;
    setsockopt(listener, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
    // Bind the socket to the address of my local machine and port number
    status = bind(listener, res->ai_addr, res->ai_addrlen);
    if (status < 0) {
        fprintf(stderr,"bind: %s\n",gai_strerror(status));
        exit(1);
    }
    
    status = listen(listener, 10);
    if(status < 0) {
        fprintf(stderr,"listen: %s\n",gai_strerror(status));
    }
    
    // Free the res linked list after we are done with it
    freeaddrinfo(res);
    
    
    // We should wait now for a connection to accept
    int new_conn_fd;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    
    // Calculate the size of the data structure
    addr_size = sizeof client_addr;
    
    std::cout << "Server listening on " << PORT << "...\n";
    
    while(1){
        // Accept a new connection and return back the socket desciptor
        new_conn_fd = accept(listener, (struct sockaddr *) & client_addr, &addr_size);
        if(new_conn_fd < 0)
        {
            fprintf(stderr,"accept: %s\n",gai_strerror(new_conn_fd));
            continue;
        }
        
        int rc;
        // Put the new clients in a new thread.
        pthread_t thread;
        rc = pthread_create(&thread, NULL, serveClient, (void *)&new_conn_fd);
        
        if (rc) {
            std::cout << "Error unable to create thread!" << rc << std::endl;
            exit(-1);
        }
        
    }
    
    return 0;
}