#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

int setupServer(int port) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, max_sd;
    int id=1;
    int CE_count=0,EE_count=0,ME_count=0,room_port;
    char buffer[1024] = {0};
    char tmp[1024];
    fd_set master_set, working_set, room_set, CE_set, EE_set, ME_set;

    if(argc==1){
        write(STDOUT_FILENO,"Error: Port Not Specified\n",27);
        return 0;
    }

    if(argc>2){
        write(STDOUT_FILENO,"Error: Too Many Arguments\n",27);
        return 0;
    }    

    server_fd = setupServer(atoi(argv[1]));
    room_port=atoi(argv[1])+1;
    FD_ZERO(&master_set);
    FD_ZERO(&CE_set);
    FD_ZERO(&EE_set);
    FD_ZERO(&ME_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(STDOUT_FILENO, "Server Started\n",16);
    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                
                if (i == server_fd) {  // new clinet
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    sprintf(tmp,"New client connected. Client Number: %d\n", new_socket);
                    write(STDOUT_FILENO,tmp,strlen(tmp));
                }
                
                else { // client sending msg
                    int bytes_received;
                    bytes_received = recv(i , buffer, 1024, 0);
                    
                    if (bytes_received == 0) { // EOF
                        sprintf(tmp,"client fd = %d closed\n", i);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        close(i);
                        FD_CLR(i, &master_set);
                        if(FD_ISSET(i,&CE_set)){
                            FD_CLR(i,&CE_set);
                            CE_count--;
                        }
                        if(FD_ISSET(i,&EE_set)){
                            FD_CLR(i,&EE_set);
                            EE_count--;
                        }
                        if(FD_ISSET(i,&ME_set)){
                            FD_CLR(i,&ME_set);
                            ME_count--;
                        }
                        continue;
                    }
                    if (atoi(buffer)==1){ //CE Room
                        sprintf(tmp,"client %d requested CE room\n", i);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        if(!FD_ISSET(i,&CE_set)){
                            FD_SET(i,&CE_set);
                            CE_count++;
                        }
                        sprintf(tmp,"CE room: %d/3\n", CE_count);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        //send(i,"Request Recived",28,0);
                        if(CE_count==3){
                            write(STDOUT_FILENO,"CE Room Filled\n",16);
                                for(int i = 0; i <= max_sd; i++){
                                    if(FD_ISSET(i,&CE_set)){
                                        sprintf(tmp, "%d%d",id, room_port);
                                        send(i,tmp,strlen(tmp),0);
                                        id++;
                                        FD_CLR(i,&CE_set);
                                    }
                                }
                            id=1;
                            CE_count=0;
                            room_port++;
                        }
                    }
                    if (atoi(buffer)==2){ //EE Room
                        sprintf(tmp,"client %d requested EE room\n", i);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        if(!FD_ISSET(i,&EE_set)){
                            FD_SET(i,&EE_set);
                            EE_count++;
                        }
                        sprintf(tmp,"EE room: %d/3\n", EE_count);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        //send(i,"Request Recived",28,0);
                        if(EE_count==3){
                            write(STDOUT_FILENO,"EE Room Filled\n",16);
                                for(int i = 0; i <= max_sd; i++){
                                    if(FD_ISSET(i,&EE_set)){
                                        sprintf(tmp, "%d%d",id, room_port);
                                        send(i,tmp,strlen(tmp),0);
                                        id++;
                                        FD_CLR(i,&EE_set);
                                    }
                                }
                            id=1;
                            EE_count=0;
                            room_port++;
                        }
                    }
                    if (atoi(buffer)==3){ //ME Room
                        sprintf(tmp,"client %d requested ME room\n", i);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        if(!FD_ISSET(i,&ME_set)){
                            FD_SET(i,&ME_set);
                            ME_count++;
                        }
                        sprintf(tmp,"ME room: %d/3\n", ME_count);
                        write(STDOUT_FILENO,tmp,strlen(tmp));
                        //send(i,"Request Recived",28,0);
                        if(ME_count==3){
                            write(STDOUT_FILENO,"ME Room Filled\n",16);
                                for(int i = 0; i <= max_sd; i++){
                                    if(FD_ISSET(i,&ME_set)){
                                        sprintf(tmp, "%d%d",id, room_port);
                                        send(i,tmp,strlen(tmp),0);
                                        id++;
                                        FD_CLR(i,&ME_set);
                                    }
                                }
                            id=1;
                            ME_count=0;
                            room_port++;
                        }
                    }

                    memset(buffer, 0, 1024);
                }
            }
            
        }

    }

    return 0;
}