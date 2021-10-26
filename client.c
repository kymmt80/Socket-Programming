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
    write(STDOUT_FILENO,"turn_passed\n",13);
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
        write(STDOUT_FILENO,"Error in connecting to server\n",31);
    }
    write(STDOUT_FILENO,"Connected to Server\n",21);
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
    char tmp[1049]={0};
    char* port;
    int id,cur_ask_turn=1,cur_ans_turn=2,mode=ASKING,bytes;
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    fd_set master_set, read_set, write_set;

    if (argc == 1)
    {
        write(STDOUT_FILENO,"Error: Port Not Specified\n",27);
        return 0;
    }

    if (argc > 2)
    {
        write(STDOUT_FILENO,"Error: Too Many Arguments\n",27);
        return 0;
    }

    server_fd = connectServer(atoi(argv[1]));
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(STDIN_FILENO, &master_set);
    FD_SET(server_fd, &master_set);
    write_to = server_fd;
    while (1)
    {
        read_set = master_set;
        if(cur_ans_turn==id&&mode==ANSWERING)
            alarm(10);
        bytes=select(max_sd + 1, &read_set, NULL, NULL, NULL);
        alarm(0);
        if(bytes<0){
            sprintf(buff,"User %d did not answer\n",id);
            sendto(room_fd, buff, strlen(buff), 0,(struct sockaddr *)&room_address, sizeof(room_address));
            continue;
        }
        if (FD_ISSET(server_fd, &read_set))
        {
            memset(buffer, 0, 1024);
            recv(server_fd, buffer, 1024, 0);
            if(buffer[0]=='$'){
                id=buffer[1]-'0';
                port=&buffer[2];
                fd = socket(AF_INET, SOCK_DGRAM, 0);
                setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
                setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

                room_address.sin_family = AF_INET;
                room_address.sin_port = htons(atoi(port));
                room_address.sin_addr.s_addr = inet_addr("255.255.255.255");
                if (bind(fd, (struct sockaddr *)&room_address, sizeof(room_address)) < 0)
                {
                    write(STDOUT_FILENO,"Error in Connecting to Room\n",29);
                }
                write(STDOUT_FILENO,"Connected to Room\n",19);
                room_fd = fd;
                FD_SET(room_fd, &master_set);
                max_sd = room_fd;
                write_to = room_fd;
            }else{
                sprintf(tmp,"Server: %s\n",buffer);
                write(STDOUT_FILENO,tmp,strlen(tmp));
            }
        }
        if(FD_ISSET(STDIN_FILENO, &read_set)){
            read(0, buffer, 1024);
            if(write_to==server_fd){
                send(write_to, buffer, strlen(buffer), 0);
            }else{
                if((cur_ask_turn==id&&mode!=ANSWERING)||(cur_ans_turn==id&&mode==ANSWERING)){
                    sprintf(tmp,"User %d: %s",id,buffer);
                    sendto(write_to, tmp, strlen(tmp), 0,(struct sockaddr *)&room_address, sizeof(room_address));
                }
                else{
                    write(STDOUT_FILENO,"Not Your Turn\n",15);
                }
            }
        }
        if(FD_ISSET(room_fd, &read_set)){
            recv(room_fd, buffer, 1024, 0);
            write(STDOUT_FILENO,buffer,strlen(buffer));
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
                    mode=MARK_BEST;
                }
            }else if(mode==MARK_BEST){
                if(cur_ask_turn==id){
                     send(server_fd, QandA, strlen(QandA), 0);
                }
                cur_ask_turn++;
                mode=ASKING;
            }
        }
        memset(buffer, 0, 1024);
    }

    return 0;
}