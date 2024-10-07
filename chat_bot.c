#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define HOST "::1"
#define BUFFER_SIZE 256

void print_error(char* msg){
    fprintf(stderr, "%s\n", msg);
    //perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	 if(argc < 3){
	 	 if(argc < 2) print_error("Port not provided in argv");
	 	 print_error("Mode not provided in argv");
    }
	 
	 int mode = atoi(argv[2]); //0 for chat logging   1 for report logging
	 if(mode != 0 && mode != 1){
	 	print_error("Mode should be either 0 or 1");
	 }
	 
	 
	 char* blocked_words[] = {"shit", "fuck"}; //bad words
	 int word_list_length = sizeof(blocked_words) / sizeof(blocked_words[0]);
	 //printf("words in the list %d\n", word_list_length);    
    
	 struct addrinfo hints, *result, *res;
    int socket_fd;
    
    char buffer[BUFFER_SIZE*2], write_buffer[BUFFER_SIZE*2];
    char name[BUFFER_SIZE], msg[BUFFER_SIZE], reciever_name[BUFFER_SIZE];
    int msg_bytes, msg_separator;
    
    time_t msg_time; 

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(HOST, argv[1], &hints, &result) != 0) {
		 print_error("getaddrinfo error");
    }
    
    for (res = result; res != NULL; res = res->ai_next) {
        socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if (socket_fd == -1){
            continue;
        }

        if (connect(socket_fd, res->ai_addr, res->ai_addrlen) == 0){
            break;
        }
    }
    freeaddrinfo(result);
    
    if (res == NULL) {
        print_error("failed to connect");
    }
    
    int logged_in = 0, r;
	 FILE *file;
	    
    
    for(;;){
    	memset(buffer, 0, sizeof buffer);
    	if(r = recv(socket_fd, buffer, sizeof buffer, 0) > 0){
    		
    		if(logged_in && !mode){ //PRANESIMAS zinutes saugomos faile
    			file = fopen("chat-log.txt", "a"); 
    			
    			char sub_string[11];
    			strncpy(sub_string, buffer, 10);
    			sub_string[10] = '\0';

    			if(strcmp(sub_string, "PRANESIMAS") == 0){
    				memset(write_buffer, 0, sizeof write_buffer);
    				strcpy(write_buffer, buffer+10);
    				
					fputs(write_buffer, file);
					fclose(file);
    				//printf("%s, %d\n", write_buffer, strlen(write_buffer));
    				
    				memset(name, 0, sizeof name);
    				memset(msg, 0, sizeof msg);
    				msg_separator = strchr(write_buffer, (int)':') - write_buffer;
    				strncpy(name, write_buffer, msg_separator);
    				name[msg_separator] = '\0';
    				strcpy(msg, write_buffer + msg_separator + 1);
    				printf("name: %s\nmsg: %s\n", name, msg);
    				
    				for(int i = 0; i < word_list_length; i++){
    					if(strstr(msg, blocked_words[i]) != NULL){
    						printf("rastas %s zinuteje %s", blocked_words[i], msg);
    						
    						time(&msg_time);
    						memset(write_buffer, 0, sizeof write_buffer);
    						sprintf(write_buffer, "PRIVATI%s;vardas:%s\tzinute:%slaikas:%s", reciever_name, name, msg, ctime(&msg_time));
    						
    						if(send(socket_fd, write_buffer, strlen(write_buffer), 0) == 0){
    							printf("no data sent\n");
    						}
    						else printf("sent: %s\n", write_buffer);
    					}
    				} 
    			}
    		}
    		else if(logged_in && mode){ //laukiama PRIVATI zinute
    			file = fopen("report-log.txt", "a");
    			
    			char sub_string[8];
    			strncpy(sub_string, buffer, 7);
    			sub_string[7] = '\0';

    			if(strcmp(sub_string, "PRIVATI") == 0){
    				memset(write_buffer, 0, sizeof write_buffer);
    				msg_separator = strchr(buffer, (int)';') - buffer;
					strcpy(write_buffer, buffer + msg_separator + 1);    				
    				
    				
    				fputs(write_buffer, file);
					fclose(file);
    				printf("%s, %d\n", write_buffer, strlen(write_buffer));
    			}
    			
    		}
    		else if(!logged_in){ //laukiama zinuciu ATSIUSKVARDA ir VARDASOK
    			
    			if(strcmp(buffer, "ATSIUSKVARDA\n") == 0){
    				memset(buffer, 0, sizeof buffer);
    			
    				printf("ATSIUSKVARDA: ");
    				scanf("%s", buffer);
    			
    				if(send(socket_fd, buffer, strlen(buffer), 0) == 0){
    					printf("no data sent\n");
    				}
    			}
    			else if(strcmp(buffer, "VARDASOK\n") == 0){
    				logged_in = 1;
    				printf("VARDASOK\n");
    				
    				if(mode == 0){
    					printf("kito boto vardas: ");
    					scanf("%s", reciever_name);
    				}
    			}   			
    		}
    	}
    	else if(r == -1){
    		fclose(file);
    		print_error("server ended connection");
    	}
    }
    fclose(file);
	 return 0;
}