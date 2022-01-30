#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

int const MAX_MAIN_SERVER_CONNECTIONS = 9, MAX_ROOM_SERVER = 3, NUMBER_ROOMS = 1024, MAX_BUFF = 1024;
  

struct Room 
{
    int count_people;
    int fd_people[3];
    int id_course;
};

char* find_course(int id)
{
    if(id == 0)
        return "Computer";
    else if(id == 1)
        return "Electrical";
    else if(id == 2)
        return "Civil";
    else 
        return "Mechanic";
}

int accept_client(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}

int setup_server(int port, int max_number_connect)
{
    struct sockaddr_in address;

    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));   
    listen(server_fd, max_number_connect);

    return server_fd;
}

void send_all_people_group(int fd_server, struct Room *rooms, int group_id, char* adress_port, int len_port)
{
    char dig[1024];
    for(int i = 0; i < 3; i++)
    {
        dig[0] = (char)(i + '0');
        strcat(dig, adress_port);
        write(rooms[group_id].fd_people[i], dig, len_port + 1);
    }
}

int handle_new_client(int* max_fd, fd_set* master_set, int server_fd, fd_set* working_set, struct Room* rooms)
{
    int new_socket = accept_client(server_fd);
    FD_SET(new_socket, master_set);
    if (new_socket > *max_fd)
        *max_fd = new_socket;
    write(1, "New client connected.\n", strlen("New client connected.\n"));
}



void add_person(int fd_server, int new_fd, int type_room, struct Room *rooms, int* rooms_id, int* number_rooms)
{
    int has_placed = 0;
    for(int i = 0; i < *number_rooms; i++)
    {
        if(rooms[i].id_course == type_room)
        {
            if(rooms[i].count_people < MAX_ROOM_SERVER)
            {
                rooms[i].fd_people[rooms[i].count_people] = new_fd;
                rooms[i].count_people++;
                write(1, "person add to a room\n", strlen("person add to a room\n"));
                has_placed = 1;
                if(rooms[i].count_people == MAX_ROOM_SERVER)
                {
                    char address_port[100] = "192.168.1.";
                    int len_port = 10;
                    char num[100];
                    char rev_num[100];
                    int temp = i + 1;
                    int len = 0;
                    while(temp != 0)
                    {
                        num[len] = (char)(temp % 10 + '0');
                        temp/=10;
                        len_port++;
                        len++;
                    }
                    for(int j = 0; j < len; j++) {
                        rev_num[j] = num[len - 1 - j];
                    }
                    strcat(address_port, rev_num);
                    write(1, "a group has completed\n", strlen("a group has completed\n"));
                    send_all_people_group(fd_server, rooms, i, address_port, len_port);
                }
                break;
            }
        }
    }
    if(!has_placed)
    {
        rooms[*number_rooms].count_people = 1;
        rooms[*number_rooms].id_course = type_room;
        rooms[*number_rooms].fd_people[0] = new_fd;
        write(1, "person add to a room\n", strlen("person add to a room\n"));
        *number_rooms += 1;
    }
}

void handle_event(int* max_fd, fd_set* master_set, int server_fd, fd_set* working_set, struct Room* rooms, int* rooms_id, int* number_rooms)
{
    char buffer[MAX_BUFF];
    for(int i = 0; i <= *max_fd; i++)
    {
        if(FD_ISSET(i, working_set))
        {
            if(i == server_fd)
                handle_new_client(max_fd, master_set, server_fd, working_set, rooms);
            else 
            {
                int bytes_received;
                bytes_received = recv(i , buffer, 1024, 0);
                if (bytes_received == 0) {
                    write(1, "a client closed\n", strlen("a client closed\n"));
                    rooms_id[i] = -1;
                    close(i);
                    FD_CLR(i, master_set);
                    continue;
                }
                if(rooms_id[i] == -1)
                {
                    int type_course = (int)buffer[0] - '0';
                    rooms_id[i] = type_course;
                    add_person(server_fd, i, type_course, rooms, rooms_id, number_rooms);
                }
                else 
                {
                    char path[1024] = "";
                    memset(path, 0, 1024);
                    strcat(path, find_course(rooms_id[i]));
                    strcat(path, ".txt");
                    int file_fd = open(path, O_CREAT | O_APPEND | O_RDWR, 0666);
                    write(file_fd, buffer, strlen(buffer));
                    close(file_fd);
                }
            }
        }
    }   
}


int main(int argc, char** argv)
{
    int server_fd = setup_server(atoi(argv[1]), MAX_MAIN_SERVER_CONNECTIONS);
    fd_set master_set;
    fd_set working_set;

    FD_ZERO(&master_set);

    int max_fd = server_fd;
    FD_SET(server_fd, &master_set);
    int number_rooms = 0;

    struct Room rooms[NUMBER_ROOMS];
    int rooms_id[NUMBER_ROOMS];
    for(int i = 0; i < NUMBER_ROOMS; i++)
        rooms_id[i] = -1;

    while(1)
    {
        working_set = master_set;
        select(max_fd + 1, &working_set, NULL, NULL, NULL);
        handle_event(&max_fd, &master_set, server_fd, &working_set, rooms, rooms_id, &number_rooms); 
    }
}