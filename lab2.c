#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int sig) {
    wasSigHup = 1;
}

void setupSignalHandler() {
    struct sigaction sa;
    
    sigaction(SIGHUP, NULL, &sa);
    
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART; 
    
    sigaction(SIGHUP, &sa, NULL);
    
    printf("Signal handler for SIGHUP registered\n");
}

int createServerSocket() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    return server_fd;
}

int main() {
    int server_fd, max_fd;
    int client_sockets[MAX_CLIENTS] = {0};
    int current_client = -1; 
    fd_set read_fds;
    sigset_t blocked_mask, orig_mask;
    char buffer[BUFFER_SIZE];
    
    setupSignalHandler();
    
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask) < 0) {
        perror("sigprocmask failed");
        exit(EXIT_FAILURE);
    }
    printf("Signal SIGHUP blocked\n");
    
    server_fd = createServerSocket();
    max_fd = server_fd;
    
    printf("Server started. PID: %d\n", getpid());
    printf("Send SIGHUP with: kill -HUP %d\n", getpid());
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        if (current_client != -1) {
            FD_SET(current_client, &read_fds);
            if (current_client > max_fd) {
                max_fd = current_client;
            }
        }
        
        int activity = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &orig_mask);
        
        if (activity < 0) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("\nReceived SIGHUP signal\n");
                    wasSigHup = 0; 
                }
                continue;
            } else {
                perror("pselect failed");
                break;
            }
        }
        
        if (wasSigHup) {
            printf("\nReceived SIGHUP signal (during pselect)\n");
            wasSigHup = 0;
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_socket;
            
            if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                perror("accept failed");
                continue;
            }
            
            printf("New connection from %s:%d\n", 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
            
            if (current_client == -1) {
                current_client = new_socket;
                printf("Keeping this connection (socket %d)\n", new_socket);
            } else {
                printf("Closing new connection (socket %d), already have active client\n", new_socket);
                close(new_socket);
            }
        }
        
        if (current_client != -1 && FD_ISSET(current_client, &read_fds)) {
            ssize_t bytes_read;
            
            bytes_read = recv(current_client, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Received %zd bytes from client (socket %d)\n", bytes_read, current_client);
                
                
            } else if (bytes_read == 0) {
                printf("Client disconnected (socket %d)\n", current_client);
                close(current_client);
                current_client = -1;
            } else {
                perror("recv failed");
                close(current_client);
                current_client = -1;
            }
        }
    }
    
    if (current_client != -1) {
        close(current_client);
    }
    close(server_fd);
    
    printf("Server stopped\n");
    return 0;
}