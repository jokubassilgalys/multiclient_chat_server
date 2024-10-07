#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define HOST "::1"
#define BUFFER_SIZE 256
#define MAX_USERS 10

void print_error(char* msg){
    fprintf(stderr, "%s\n", msg);
    //perror(msg);
    exit(EXIT_FAILURE);
}

struct client_data {
    int descriptor;
    char name[BUFFER_SIZE];
};

void remove_client(struct client_data clients[MAX_USERS], int descriptor, int *client_count){
	for(int i = 0; i < *client_count; i++){
		if(clients[i].descriptor == descriptor){
			for(int j = i; j < *client_count - 1; j++){
				clients[j] = clients[j + 1];
			}
			*client_count = *client_count-1;
			break;
		}
	}
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        print_error("Port not provided in argv");
    }

    struct client_data clients[MAX_USERS];
    int client_count = 0;

    fd_set master_fd, read_fd;
    int fdmax;

    FD_ZERO(&master_fd);
    FD_ZERO(&read_fd);

    struct addrinfo hints, *result, *res;
    int listener, yes = 1, r;

    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    int client_fd;

    char buffer[BUFFER_SIZE], send_buffer[BUFFER_SIZE*2];
    int msg_bytes;
   

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;         
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, argv[1], &hints, &result) != 0) {
        print_error("getaddrinfo error");
    }  

    for (res = result; res != NULL; res = res->ai_next) {

        listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if (listener == -1){
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, res->ai_addr, res->ai_addrlen) == 0){
            //close(listener);
            break;
        }
    }

    freeaddrinfo(result);

    if (res == NULL) {
        print_error("Failed to bind socket");
    }

    if (listen(listener, MAX_USERS) == -1){
        print_error("listen error");
    }

    FD_SET(listener, &master_fd);

    fdmax = listener;

    printf("Serveris veikia...\n");

    for(;;) {
        read_fd = master_fd;
        if (select(fdmax+1, &read_fd, NULL, NULL, NULL) == -1) {
            print_error("select error");
        }

        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fd)) {
                if (i == listener) {    //Naujas klientas
                    addr_size = sizeof client_addr;
                    client_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_size);

                    for(;;){
                        strcpy(buffer, "ATSIUSKVARDA\n");
                        send(client_fd, buffer, strlen(buffer), 0);
                        memset(buffer, 0, sizeof buffer);

                        if(recv(client_fd, buffer, sizeof buffer, 0) > 0){
                            clients[client_count].descriptor = client_fd;                         
                            
                            //buffer[strlen(buffer)-1] = '\0';
                            
                            while(buffer[strlen(buffer)-1] == '\n' || buffer[strlen(buffer)-1] == '\r'){
                            	//printf("s:%s, l:%d, c:%c\n", buffer, strlen(buffer), buffer[strlen(buffer)]);
                            	buffer[strlen(buffer)-1] = '\0';
                            }
                            
                            strcpy(clients[client_count].name, buffer);
                            client_count++;

                            memset(buffer, 0, sizeof buffer);
                            strcpy(buffer, "VARDASOK\n");
                            send(client_fd, buffer, strlen(buffer), 0);
                            memset(buffer, 0, sizeof buffer);

                            break;
                        }
                    }
                    
                    if (client_fd == -1) {
                        print_error("accept error");
                    }
                    else {
                        FD_SET(client_fd, &master_fd);
                        if (client_fd > fdmax) {
                            fdmax = client_fd;
                        }
                    }
                }
                else {  //Gauta zinute
                    memset(buffer, 0, sizeof buffer);
                    memset(send_buffer, 0, sizeof send_buffer);
                    if ((msg_bytes = recv(i, buffer, sizeof buffer, 0)) <= 0) {
                        printf("socket %d disconnected\n", i);
                        close(i);
                        
                        remove_client(clients, i, &client_count);
                        printf("client count: %d\n", client_count);
                        
                        FD_CLR(i, &master_fd);
                 			if(client_count == 0){
                 				print_error("All users have disconnected");
                 			}
                    }
                    else {
                    	   char sub_string[8];
    							strncpy(sub_string, buffer, 7);
    							sub_string[7] = '\0';

    							if(strcmp(sub_string, "PRIVATI") == 0){
    								
    								int msg_separator = strchr(buffer, (int)';') - buffer;
    								char reciever_name[BUFFER_SIZE];
    								strncpy(reciever_name, buffer+7, msg_separator-7);
    								reciever_name[msg_separator] = '\0';
    								
    								for(int k = 0; k < client_count; k++){
    									//printf("v1: %s, v2: %s\n", clients[k].name, reciever_name);
	    								if(strcmp(clients[k].name, reciever_name) == 0){
	    									send(clients[k].descriptor, buffer, strlen(buffer), 0);
	    								}
    								}
    								continue;
    							}

                    		for(int k = 0; k < client_count; k++){
                    			if(clients[k].descriptor == i){
                        		strcat(send_buffer, clients[k].name);
                        		sprintf(send_buffer, "PRANESIMAS%s:%s", clients[k].name, buffer);
                        		break;
                    			}
                    		}
                    				
                    		for(int j = 0; j < client_count; j++) {
                        	printf("zinute:%s, klientas:%d\n", send_buffer, clients[j].descriptor);
                       		send(clients[j].descriptor, send_buffer, strlen(send_buffer), 0);
                        }
                    }
                }
            }
        }
    } 

    return 0;
}

