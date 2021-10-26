#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define ASKING 0
#define ANSWERING 1
#define MARK_BEST 2

void alarm_handler(int sig){
    printf("turn_passed\n");
}

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}

struct sockaddr_in bc_address;

int connectRoom(int port)
{
    int fd;
    struct sockaddr_in room_address;
    int broadcast = 1, opt = 1;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    room_address.sin_family = AF_INET;
    room_address.sin_port = htons(port);
    room_address.sin_addr.s_addr = inet_addr("255.255.255.255");
    bc_address=room_address;
    if (bind(fd, (struct sockaddr *)&room_address, sizeof(room_address)) < 0)
    {
        printf("Error in connecting to room\n");
    }
    printf("connected to room\n");
    //if (connect(fd, (struct sockaddr *)&room_address, sizeof(room_address)) < 0) { // checking for errors
    //    printf("Error in connecting to server\n");
    //}

    return fd;
}

int main(int argc, char const *argv[])
{
    int fd;
    struct sockaddr_in room_address;
    int broadcast = 1, opt = 1;
    int server_fd, room_fd=-1, max_sd, write_to;
    char buff[1049] = {0};
    char buffer[1024] = {0};
    char QandA[1024]={0};
    char* port;
    int id,cur_ask_turn=1,cur_ans_turn=2,mode=ASKING,bytes;
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    fd_set master_set, read_set, write_set;

    if (argc == 1)
    {
        printf("Error: Port Not Specified\n");
        return 0;
    }

    if (argc > 2)
    {
        printf("Error: Too Many Arguments\n");
        return 0;
    }

    server_fd = connectServer(atoi(argv[1]));
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(STDIN_FILENO, &master_set);
    FD_SET(server_fd, &master_set);
    write_to = server_fd;
    printf("connected\n");
    while (1)
    {
        read_set = master_set;
        if(cur_ans_turn==id&&mode==ANSWERING)
            alarm(10);
        bytes=select(max_sd + 1, &read_set, NULL, NULL, NULL);
        alarm(0);
        if(bytes<0){
            sprintf(buff,"User %d did not answer\n",id);
            sendto(room_fd, buff, strlen(buff), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
            continue;
        }
        if (FD_ISSET(server_fd, &read_set))
        {
            printf("message from:%d\n", server_fd);
            memset(buffer, 0, 1024);
            recv(server_fd, buffer, 1024, 0);
            id=buffer[0]-'0';
            port=&buffer[1];
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

            room_address.sin_family = AF_INET;
            room_address.sin_port = htons(atoi(port));
            room_address.sin_addr.s_addr = inet_addr("255.255.255.255");
            bc_address=room_address;
            if (bind(fd, (struct sockaddr *)&room_address, sizeof(room_address)) < 0)
            {
                printf("Error in connecting to room\n");
            }
            printf("connected to room\n");
            room_fd = fd;
            printf("%d\n", atoi(buffer));
            FD_SET(room_fd, &master_set);
            max_sd = room_fd;
            write_to = room_fd;
        }
        if(FD_ISSET(STDIN_FILENO, &read_set)){
            //write(0, buffer, strlen(buffer));
            //write(0,buffer,strlen(buffer));
            //printf("message from:%d\n", STDIN_FILENO);
            read(0, buffer, 1024);
            if(write_to==server_fd){
                send(write_to, buffer, strlen(buffer), 0);
            }else{
                //printf("message to 4\n");
                if((cur_ask_turn==id&&mode!=ANSWERING)||(cur_ans_turn==id&&mode==ANSWERING)){
                    sprintf(buff,"User %d: %s",id,buffer);
                    sendto(write_to, buff, strlen(buff), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
                }
                else{
                    printf("Not Your Turn\n");
                }
            }
        }
        if(FD_ISSET(room_fd, &read_set)){
            //write(0, buffer, strlen(buffer));
            //write(0,buffer,strlen(buffer));
            //printf("message from:%d\n", room_fd);
            recv(room_fd, buffer, 1024, 0);
            printf("%s\n",buffer);
            if(cur_ask_turn==id){
                strcat(QandA,buffer);
            }
            if(mode==ASKING){
                mode=ANSWERING;
                cur_ans_turn=(cur_ask_turn)%3+1;
            }
            else if(mode==ANSWERING){
                cur_ans_turn=(cur_ans_turn)%3+1;
                if(cur_ans_turn==cur_ask_turn){
                    //if(cur_ask_turn==id){
                    //    send(server_fd, QandA, strlen(QandA), 0);
                    //}
                    //cur_ask_turn++;
                    //mode=ASKING;
                    mode=MARK_BEST;
                }
            }else if(mode==MARK_BEST){
                if(cur_ask_turn==id){
                     send(server_fd, QandA, strlen(QandA), 0);
                }
                cur_ask_turn++;
                mode=ASKING;
            }
            //printf("User %d turn in %d mode\n",cur_ans_turn,mode);
        }
        memset(buffer, 0, 1024);
        // }
    }

    return 0;
}