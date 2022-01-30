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

#define DEFUALT_IP_ADDRESS "127.0.0.1"
#define EMPTY_STR '\0'

int connect_to_server(int port) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr(DEFUALT_IP_ADDRESS);

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Error in connecting to server\n");
    }

    return fd;
}

void ask_question(int fd, int sock,  struct sockaddr_in *bc_address, int id_int, int first_person, int second_person)
{
    char question[1024];
    char answer[1024] = "";
    char answer2[1024] = "";
    char id_answer[1024];
    char temp[1024];
    write(1, "Please ask your question\n", strlen("Please ask your question\n"));
    read(0, question, 1024);
    sendto(sock, question, strlen(question), 0,(struct sockaddr *)bc_address, sizeof(*bc_address));
    recv(sock, temp, 1024, 0);
    memset(answer, 0, 1024);
    recv(sock, answer, 1024, 0);
    write(1, "first answer is: \n", strlen("first answer is: \n"));
    write(1, answer, strlen(answer));
    memset(answer2, 0, 1024);
    recv(sock, answer2, 1024, 0);
    write(1, "second answer is: \n", strlen("second answer is: \n"));
    write(1, answer2, strlen(answer2));
    write(1, "which answer is the best? 1 or 2 \n", strlen("which answer is the best? 1 or 2 \n"));
    read(0, id_answer, 1024);
    if(id_answer[0] == '1')
    {
        answer[strlen(answer) - 1] = '*';
    }
    else 
    {
        answer[strlen(answer) - 1] = ' ';
        answer2[strlen(answer2) - 1] = '*';
    }
    strcat(answer, "\n");
    strcat(answer, answer2);
    strcat(question, answer);
    send(fd, question, strlen(question), 0);
}

void alarm_handler()
{
    write(1, "Your time is over!\n", strlen("Your time is over!\n"));
}

void answer_question(int sock,  struct sockaddr_in *bc_address, int id_int, int id_asked, int first_person, int second_person)
{
    char question[1024];
    char answer[1024];
    char temp[1024];
    memset(question, 0, 1024);
    recv(sock, question, 1024, 0);
    write(1, "question is: \n", strlen("question is: \n"));
    write(1, question, strlen(question));
    if(first_person != id_int)
    {
        recv(sock, temp, 1024, 0);
        write(1, "you can answer now\n", strlen("you can answer now\n"));
    }
    else 
    {
        write(1, "you can answer now\n", strlen("you can answer now\n"));
    }
    signal(SIGALRM, alarm_handler);
    siginterrupt(SIGALRM, 1);
    alarm(60);
    int read_ret = read(0, answer, 1024);
    alarm(0);
    if(answer[0] == EMPTY_STR)
    {
        sendto(sock, "Don't have any answer!\n", strlen("Don't have any answer!\n"), 0,(struct sockaddr *)bc_address, sizeof(bc_address));
    }
    else 
        sendto(sock, answer, strlen(answer), 0,(struct sockaddr *)bc_address, sizeof(*bc_address));
    recv(sock, temp, 1024, 0);
    if(first_person == id_int)
    {
        recv(sock, temp, 1024, 0);
    }
}

int main(int argc, char const *argv[]) {
    int fd;
    char buff[1024] = {0};
    char address_port[1024];
    char sentence_get[1024];
    char id_now[1024];

    fd = connect_to_server(atoi(argv[1]));
    read(0, buff, 1024);
    send(fd, buff, strlen(buff), 0);
    memset(buff, 0, 1024);
    int bytes_recieved = recv(fd, address_port, 1024, 0);
    int id_int = (int)(address_port[0] - '0');
    id_now[0] = address_port[0];
    for(int i = 0; i < bytes_recieved + 1; i++)
        address_port[i] = address_port[i + 1];
    address_port[bytes_recieved - 2] = EMPTY_STR;
    char str_out_port[1024] = "recieved port number which is: ";
    strcat(str_out_port, address_port);
    strcat(str_out_port, " and my id is: ");
    strcat(str_out_port, id_now);
    strcat(str_out_port, "\n");
    write(1, str_out_port, strlen(str_out_port));
    

    int sock, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(atoi(argv[1])); 
    bc_address.sin_addr.s_addr = inet_addr(address_port);

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    for(int i = 0; i < 3; i++)
    {
        int first_person = 0;
        int second_person = 1;
        if(i == 0)
        {
            first_person = 1;
            second_person = 2;
        }
        if(i == 1)
        {
            second_person = 2;
        }
        if(i == id_int)
        {
            ask_question(fd, sock, &bc_address, id_int, first_person, second_person);
        }
        else 
            answer_question(sock, &bc_address, id_int, i, first_person, second_person);
    }
    return 0;
}