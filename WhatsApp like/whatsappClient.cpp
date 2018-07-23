/**
 * @author Shlomie Berg
 */

#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <regex>
#include "whatsappio.h"


#define MAX_MSG 256
#define ARGS_NUM 4
#define NULLC '\0'
#define CONNECTION_FAILED "Failed to connect the server\n"
#define NAME_IS_TAKEN "Client name is already in use.\n"


std::string clientName;

/**
 * validates weather name is a valid name.
 * @param name
 * @return
 */
bool nameCheck (std::string name)
{
    std::regex re("(([0-9])|([A-Z])|([a-z]))+");
    return std::regex_match(name, re);
}

/**
 * Gets the message from server
 * @param sfd
 * @param buf
 */
void getServerMsg(int sfd, char *buf){
    int bcount; // counts bytes read
    int br; // bytes read this pass
    bcount = 0; br = 0;

    char *traveler = buf;
    char checkLastChar = NULLC;
    while (checkLastChar != '\n') { // loop until we reach the end of the message.
        br = (int)read(sfd, traveler, MAX_MSG - bcount);
        if (br > 0) {
            bcount += br;
            traveler += br;
        }
        else if(br == 0) {
            close(sfd);
            exit(1);
        }
        else{
            print_error(READ_FUNC, errno);
            close(sfd);
            exit(1);
        }
        checkLastChar = buf[bcount-1]; // check if the last character is '\n'
    }
    std::string msg = buf;
    if (!msg.compare("exit\n")) { // In case the server is shutting down.
        close(sfd);
        exit(1);
    }
    else
        std::cout << buf;
}

/**
 * validates the server's response.
 * @param sfd
 * @param buf
 */
void validateServerResponse(int sfd, char *buf){
    std::string checkMsg = buf;
    if (!checkMsg.compare(NAME_IS_TAKEN) || !checkMsg.compare(CONNECTION_FAILED)){
        close(sfd);
        exit(1);
    }
}

/**
 * Handles the create group command.
 * @param sfd
 * @param name
 * @param clients
 */
void createGroup(int sfd, const std::string name, const std::vector<std::string> clients)
{
    if(!nameCheck(name)){
        print_create_group(false, false, clientName, name);
        return;
    }
    std::string clientNames = "";
    for (std::string client : clients){
        if (!name.compare(client)) { // Group name equals one of the client names
            print_create_group(false, false, clientName, name);
            return;
        }
        if (!nameCheck(client)){
            print_create_group(false, false, clientName, name);
            return;
        }
        clientNames += client + ",";
    }

    if(!clientNames.empty())
        clientNames = clientNames.substr(0,clientNames.length()-1);
    else { // No clients
        print_create_group(false, false, clientName, name);
        return;
    }

    if(!clientNames.compare(clientName)){ // Checks if there are only one user.
        print_create_group(false, false, clientName, name);
        return;
    }

    std::string serverRequest = "create_group " + name + " " +clientNames + "\n";
    if (send(sfd, serverRequest.c_str(), serverRequest.length(), 0) < 0) {
        print_error(SEND_FUNC, errno);
        close(sfd);
        exit(1);
    }

    // Wait for server response.
    char msg [MAX_MSG];
    memset(msg, 0, sizeof(msg));
    getServerMsg(sfd, msg);
}

/**
 * Handles the WHO command.
 * @param sfd
 * @param name
 * @param message
 */
void clientSend(int sfd, std::string name, std::string message){
    if(!name.compare(clientName)){
        print_send(false, false, clientName, name, message);
        return;
    }
    std::string serverRequest = "send " + name + " " + message + "\n";
    if (send(sfd, serverRequest.c_str(), serverRequest.length(), 0) < 0) {
        print_error(SEND_FUNC, errno);
        close(sfd);
        exit(1);
    }

    // Wait for server response.
    char msg [MAX_MSG];
    memset(msg, 0, sizeof(msg));
    getServerMsg(sfd, msg);

}

/**
 * Sends who request to the server.
 * @param sfd
 */
void clientWho(int sfd){
    std::string serverRequest = "who\n";
    if (send(sfd, serverRequest.c_str(), serverRequest.length(), 0) < 0) {
        print_error(SEND_FUNC, errno);
        close(sfd);
        exit(1);
    }

    // Wait for server response.
    char msg [MAX_MSG];
    memset(msg, 0, sizeof(msg));
    getServerMsg(sfd, msg);
}

/**
 * Handles the client exit command.
 * @param sfd
 */
void clientExit(int sfd){
    std::string serverRequest = "exit\n";
    if (send(sfd, serverRequest.c_str(), serverRequest.length(), 0) < 0) {
        print_error(SEND_FUNC, errno);
        close(sfd);
        exit(1);
    }

    // Wait for server response.
    char msg [MAX_MSG];
    memset(msg, 0, sizeof(msg));
    getServerMsg(sfd, msg);

    close(sfd);
    exit(0);
}

/**
 * manage the process of sending the user command to the server.
 * @param sfd
 * @param command
 */
void sendUserCommand(int sfd, std::string command) {
    const std::string uCommand = command.substr(0, command.find("\n"));
    command_type commandT;
    std::string name;
    std::string message;
    std::vector<std::string> clients;

    parse_command(uCommand, commandT, name, message, clients);

    switch (commandT) {
        case CREATE_GROUP:
            createGroup(sfd, name, clients);
            break;
        case SEND:
            clientSend(sfd, name, message);
            break;
        case WHO:
            clientWho(sfd);
            break;
        case EXIT:
            clientExit(sfd);
            break;
        case INVALID:
            print_invalid_input();
            break;
    }
}

/**
 * listens to server while waits for a new message.
 * @param sfd
 */
void listenToServer(int sfd){

    fd_set clientfds;
    fd_set readfds;

    // Initialize sets.
    FD_ZERO(&clientfds);
    FD_ZERO(&readfds);

    FD_SET(sfd, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds);

    while (true)
    {
        readfds = clientfds;

        if (select(sfd + 1, &readfds, NULL, NULL, NULL) < 0) // System call error
        {
            print_error(SELECT_FUNC, errno);
            close(sfd);
            exit(1);
        }

        if (FD_ISSET(sfd, &readfds)) { // check if the server sent something.
            char msg [MAX_MSG];
            memset(msg, 0, sizeof(msg));
            getServerMsg(sfd, msg);
        }

        else if (FD_ISSET(STDIN_FILENO, &readfds)) { // check if user sent something.
            std::string command;
            getline(std::cin, command);
            sendUserCommand(sfd, command);
        }

    }

}

int main(int argc, char *argv[]){

    // input check
    if(argc != ARGS_NUM){
        print_invalid_input();
        return (-1);
    }

    struct sockaddr_in sa;
    struct hostent* hp;

    int sfd; // socket file descriptor.
    clientName = argv[1];
    char* getAddr = argv[2];
    char *port = argv[3];
    char msg[MAX_MSG];

    if((hp = gethostbyname(getAddr)) == NULL) {
        print_error(GETHOSTBY_FUNC,errno);
        exit(1);
    }

    // initialize sa
    memset(&sa, 0, sizeof(sa));
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)atoi(port));


    // create socket
    if ((sfd = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0) {
        print_error(SOCKET_FUNC,errno);
        exit(1);
    }


    // connect to the server
    if (connect(sfd, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        print_error(CONNECT_FUNC,errno);
        print_fail_connection();
        close(sfd);
        exit(1);
    }

    // send the name to the server
    std::string name_msg = "name " + clientName + "\n";
    if ((send(sfd, name_msg.c_str(), name_msg.length(), 0)) < 0) {
        print_error(SEND_FUNC,errno);
        close(sfd);
        exit(1);
    }

    memset(msg, 0, sizeof(msg)); // initialize msg buffer.

    // get server reponse
    getServerMsg(sfd, msg);

    validateServerResponse(sfd, msg);

    listenToServer(sfd);

    close(sfd);
    return 0;


}