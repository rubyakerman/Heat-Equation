/**
 * @author Roy Ackerman
 */

#include <iostream>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "whatsappio.h"
#include <set>
#include <map>
#include <sstream>

using namespace std;

fd_set clientFDSet;
int server_socket_fd;

//////////////////////////////////////
//      Data structures             //
//////////////////////////////////////
vector<int> clients_fds; // the fd for our clients
map<string, int> clients_per_fds; // keeps client name and its fd
map<string, set<string>> groups_per_clients; // keeps for each group its clients

//////////////////////////////////////
//              Constants           //
//////////////////////////////////////
const int DEFAULT_PROTOCOL = 0;
const int SERVER_ARGS_NUMBER = 2;
const int GROUP_NAME = 2;
const int CLIENT_NAME = 1;
const int NAME_NOT_EXISTS = -1;


//////////////////////////////////////
//              Methods             //
//////////////////////////////////////

/**
 * reports for an error.
 * @param msg
 */
void report_error(string  msg) {
    cout << "ERROR: " << msg << " " << errno <<"\n";
}

/**
 * checks weather client_command is an exit command
 * @param client_command
 * @return
 */
bool is_exit_command(string client_command){
    return (client_command.compare("exit") == 0);
}

/**
 * checks weather client_command is a send command
 * @param client_command
 * @return
 */
bool is_send_command(string client_command){return (!client_command.compare("send"));}

/**
 * checks weather client_command is a create group command
 * @param client_command
 * @return
 */
bool is_create_group_command(string client_command){return !(client_command.compare("create_group"));}

/**
 * checks weather client_command is a who command
 * @param client_command
 * @return
 */
bool is_who_command(string client_command){return !(client_command.compare("who"));}

/**
 * Removes the client from groups_per_clients.
 * @param client_name
 */
void remove_from_groups_per_clients(string client_name){

    for (map<std::string, set<std::string>> :: iterator  mi = groups_per_clients.begin(); !(mi == groups_per_clients.end()) ; ++ mi) {
        for (set<std::string> :: iterator si = (*mi).second.begin(); !(si == (*mi).second.end()); ++ si) {
            bool client_name_founded = ((*si).compare(client_name)) == 0;
            if(client_name_founded){
                (*mi).second.erase(*si);
                break;
            }
        }
    }
}

/**
 * Removes the client from clients_fds.
 */
void remove_fd_from_clients_fds(int client_fd){
    vector<int>:: iterator fd_i = find(clients_fds.begin(), clients_fds.end(), client_fd);
    clients_fds.erase(fd_i);
}

/**
 * Returns the client name that belongs to client_fd fd.
 */
string getClientName(int client_fd){
    const string NOT_FOUND = "";

    for( map<std:: string, int>:: iterator mi = clients_per_fds.begin(); !(mi == clients_per_fds.end()); ++ mi ){
        if((*mi).second == client_fd){return (*mi).first;}
    }

    return NOT_FOUND;
}


/**
 * Shuts down the server (user exit command).
 * @param client_fd
 */
void exit_command(int client_fd){

    string client_name = getClientName(client_fd);
    remove_from_groups_per_clients(client_name);

    clients_per_fds.erase(client_name);
    remove_fd_from_clients_fds(client_fd);
    FD_CLR(client_fd, &clientFDSet);

    string msg = client_name + ": Unregistered successfully.\n";;

    if(send(client_fd, msg.c_str(), msg.length(), 0) < 0){
        report_error(SEND_FUNC);
    }

    print_exit(true, client_name);
}

/**
 * @param client_name
 * @return true if exists a client with the name client_name, false otherwise
 */
bool is_client_name(string client_name){
    return (clients_per_fds.find(client_name) != clients_per_fds.end());
}

/**
 * @param client_name
 * @return true if exists a group with the name client_name, false otherwise
 */
bool is_group_name(string client_name){
    return (groups_per_clients.find(client_name) != groups_per_clients.end());
}

/**
 * check weather the clinet c_receiver_name exists or not
 * @param c_receiver_name
 * @return:
 * -1 if not exists
 * 1 is a client name
 * 2 is a group name
 */
int identify_name(string c_receiver_name) {
    if (is_group_name(c_receiver_name)) {
        return GROUP_NAME;
    } else if (is_client_name(c_receiver_name)) {
        return CLIENT_NAME;
    } else { return NAME_NOT_EXISTS; }
}

/**
 * Checks weather the clint name "member" is a member of the group "group".
 * @param member_name - client name
 * @param group_name - group name
 */
bool is_group_member(string member_name, string group_name){
    map<std::string, set<std::string>> :: iterator mi = groups_per_clients.find(group_name);

    if (mi == groups_per_clients.end()){ // Group is not exist
        return false;
    }else
    { // Group was found:
        set<std::string> group_set = mi->second;
        bool is_found = (group_set.find(member_name) != group_set.end());
        return is_found;
    }
}

/**
 * Sends the message to the client c_receiver_name.
 * @param sender_fd
 * @param c_sender_name
 * @param message
 * @param c_receiver_name
 */
void sendToClient(int sender_fd, string c_sender_name, string message, string c_receiver_name){
    string msg_to_send = c_sender_name + ": ";
    msg_to_send += message + "\n";

    // Send message to the receiver client:
    if (send(clients_per_fds[c_receiver_name],msg_to_send.c_str(), msg_to_send.length(),0) < 0) {
        report_error(SEND_FUNC);
        return;
    }
    print_send(true, true, c_sender_name, c_receiver_name, message);

    msg_to_send = "Sent successfuly.\n";
    // send "successfully sent" message to sender
    if (send(sender_fd, msg_to_send.c_str(), msg_to_send.length(),0) < 0) {
        report_error(SEND_FUNC);
        return;
    }

}

/**
 * Sends the message to each one of the group's members
 * @param sender_fd
 * @param c_sender_name
 * @param message
 * @param c_receiver_name
 */
void sendToMembers(int sender_fd, string c_sender_name, string message, string c_receiver_name){
    string message_to_target = c_sender_name + ": ";
    message_to_target +=  message + "\n";
    for(set<std::string> :: iterator si = groups_per_clients[c_receiver_name].begin(); si != groups_per_clients[c_receiver_name].end(); ++ si ){
        if (c_sender_name.compare(*si) != 0){ // don't send the message to the sender
            if (send(clients_per_fds[*si], message_to_target.c_str(), message_to_target.length(), 0) < 0) {
                report_error(SEND_FUNC);
                return ;
            }
        }
    }

    // Sends a "success message" to the sender client:
    string message_to_client = "Sent successfully.\n";
    if (send(sender_fd, message_to_client.c_str(), message_to_client.length(), 0) < 0) {
        report_error(SEND_FUNC);
        return;
    }

    // print successful sent message for the server
    print_send(true, true, c_sender_name, c_receiver_name, message);
}

/**
 * taking care of the sending operation to a group.
 * @param sender_fd
 * @param c_sender_name
 * @param message
 * @param c_receiver_name
 * @param error
 */
void sendToGroup(int sender_fd, string c_sender_name, string message, string c_receiver_name, string error){

    if (is_group_member(c_sender_name, c_receiver_name)){ // is c_sender_name a member of the group c_receiver_name?
        // Sends message_to_target to each one of the group's members:
        sendToMembers(sender_fd, c_sender_name, message,c_receiver_name);
    }
    else{ // c_sender_name is'nt a member of the group c_receiver_name
        print_send(true, false, c_sender_name, c_receiver_name, message);

        if (send(sender_fd, error.c_str(), error.length(), 0) < 0 ){
            report_error(SEND_FUNC);
            return;
        }
    }

}



/**
 * handles the send user send command
 */
void send_command(int client_fd, string user_input){

    int sender_fd = client_fd;

    const std::string uCommand = user_input.substr(0, user_input.find("\n"));
    command_type commandT;
    std::string name;
    std::string message;
    std::vector<std::string> clients;

    parse_command(uCommand, commandT, name, message, clients);

    string c_sender_name = getClientName(sender_fd);
    string c_receiver_name = name;
    string error = "ERROR: failed to send.\n";

    int target = identify_name(c_receiver_name);

    if(target == CLIENT_NAME)
    {
        sendToClient(sender_fd, c_sender_name, message, c_receiver_name);
    }
    else if(target == GROUP_NAME) // receiver is a group name
    {
        sendToGroup(sender_fd, c_sender_name, message, c_receiver_name, error);
    }
    else // target not exists
    {
        print_send(true, false, c_sender_name, c_receiver_name, message);

        if (send(sender_fd, error.c_str(), error.length(), 0) < 0) {
            report_error(SEND_FUNC);
            return;
        }
    }
}

/**
 *  Validate the member "member" is indeed a member of the group "group",
 *  Validate the group contains more members than just him.
 * @param group - group name
 * @param member - member name
 * @param group_members - string contains the group members (from user's input)
 * @param member_fd (fd of the member)
 * @return true if valid, otherwise false.
 */
bool is_valid_group(string group, string  member, string group_members, int member_fd){

    string error = "ERROR: failed to create group \"" + group + "\".\n";
    bool group_has_no_members = ((group_members.compare(member) == 0) || group_members.empty());

    if ((identify_name(group) != NAME_NOT_EXISTS) || group_has_no_members){
        if(send(member_fd,error.c_str(),error.length(),0) < 0){
            report_error(SEND_FUNC);
        }
        return false;
    }
    return true;
}

/**
 * Prints a "group was created successfully" message
 * to the client & the server as well.
 * @param group
 * @param members
 * @param c_sender
 * @param sender_fd
 */
void printSuccessMSG(string group, set<string> members, string c_sender, int sender_fd){

    groups_per_clients[group] = members;

    string message = "Group \"" + group + "\" was created successfully.\n";
    print_create_group(true, true, c_sender, group);

    if (send(sender_fd,message.c_str(),message.length(),0) < 0) {
        report_error(SEND_FUNC);
    }
}

/**
 * Creates a new group.
 * @param c_sender - the client who wants to create the group.
 * @param group_members - its members.
 * @param group - its name.
 */
void create_new_group(string c_sender, string group_members, string group){

    string error = "ERROR: failed to create group "+group+"\".\n";;
    int sender_fd = clients_per_fds[c_sender];
    set<string> members;
    istringstream sStream(group_members);
    string next_token;

    while(getline(sStream, next_token, ',') )
    {
        // Checks if the next member is an existing client - add it to the group if is:
        if (is_client_name(next_token)){
            members.insert(next_token);
        }
        else{ // Next member is'nt a client of the server - report error
            if (send(sender_fd, error.c_str(), error.length(), 0) < 0) {
                report_error(SEND_FUNC);
            }
            return;
        }
    }
    members.insert(c_sender); // Appending the sender to the group.

    // Prints success messages: for sender & server
    printSuccessMSG(group, members, c_sender, sender_fd);

}

/**
 * Handles the "create_group" command
 * @param sender_fd - FD number, represents the connection file with the client
 * @param user_input - the user's input (the rest of it).
 */
void create_group_command(int sender_fd, string user_input){

    const std::string uCommand = user_input.substr(0, user_input.find("\n"));
    command_type commandT;
    std::string group_name;
    std::string message;
    std::vector<std::string> clients;

    parse_command(uCommand, commandT, group_name, message, clients);

    string group_members = "";
    for(string member : clients){
        group_members += member;
        group_members += ",";
    }

    group_members = group_members.substr(0,group_members.length()-1);

    string c_sender = getClientName(sender_fd);
    sender_fd = clients_per_fds[c_sender];

    if(!is_valid_group(group_name, c_sender, group_members, sender_fd)){
        print_create_group(true, false, c_sender, group_name);
        return;
    }

    // Creating the group:
    create_new_group(c_sender, group_members, group_name);
}

/**
 * Handles the "who" command from client its fd is client_fd
 * @param client_fd
 */
void who_command(int client_fd){
    string client_sender_name;
    string members = "";

    // Inform the server:
    client_sender_name = getClientName(client_fd);
    print_who_server(client_sender_name);

    // Grab available (connected) clients
    for(map<std::string, int> :: iterator ci = clients_per_fds.begin(); ci != clients_per_fds.end(); ++ ci){
        members += (*ci).first + ",";
    }
    members = members.substr(0, members.length()-1) + "\n";

    client_fd = clients_per_fds[client_sender_name];
    if (send(client_fd, members.c_str(), members.length(), 0) < 0){
        report_error(SEND_FUNC);
    }
}

/**
 * Connects (appends) a new client to the server.
 * @param client_fd
 * @param client_input
 */
void new_client_command(string c_name,int client_fd){

    string name = c_name.substr(0, c_name.find("\n"));
    string used_name_msg;

    // Name already exists (as a client or as a group)
    if (identify_name(name) != NAME_NOT_EXISTS) {

        used_name_msg = "Client name is already in use.\n";
        if (send(client_fd, used_name_msg.c_str(), used_name_msg.length(), 0) < 0) {
            report_error(SEND_FUNC);
        }
        // Remove the client_fd from everywhere:
        FD_CLR(client_fd, &clientFDSet);
        vector <int> :: iterator fdI = find(clients_fds.begin(), clients_fds.end(), client_fd);
        clients_fds.erase(fdI);

    }
    else // This is new client that is'nt exist - add it
    {
        clients_per_fds[name] = client_fd;
        print_connection_server(name);

        string msg_to_client = "Connected successfully.\n";
        if (send(client_fd, msg_to_client.c_str(), msg_to_client.length(), 0) < 0) {
            report_error(SEND_FUNC);
        }
    }

}

/**
 * takes care of handling user input
 * @param client_fd
 * @param client_input
 */
void handle_client_input(int client_fd ,char *client_input){

    const unsigned int START = 0;
    const string NEW_LINE = "\n";
    const string SPACE = " ";

    string client_command = client_input;
    string client_request = client_command.substr(START, client_command.find(NEW_LINE));
    client_command = client_request.substr(START, client_command.find(SPACE));

    if (is_exit_command(client_command)){
        exit_command(client_fd);
    }

    else if (is_send_command(client_command)){send_command(client_fd, client_input);}

    else if (is_create_group_command(client_command)){create_group_command(client_fd, client_input);}

    else if (is_who_command(client_command)){who_command(client_fd);}

    else // command is a new client connection request (adding a client)
    {
        string c_name = client_request.substr(5, client_request.length());
        new_client_command(c_name, client_fd);
    }
}

/**
 * checks weather we've got
 * the right number of arguments
 */
void args_validation_check(int numOfArgs){

    if (numOfArgs != SERVER_ARGS_NUMBER){
        report_error("Usage: whatsappServer portNum ");
        exit(1);
    }
}

/**
 *  This function returns a hostent struct
 *  filled with the localhost information
 */
hostent* get_localhost(){
    char localhost_name[WA_MAX_NAME];
    struct hostent *new_host;

    if(gethostname(localhost_name,WA_MAX_NAME) == - 1){ // Receives the name of the host into localhost_name.
        report_error(GETHOST_FUNC);
    }

    new_host = gethostbyname(localhost_name);
    if(new_host == nullptr){
        report_error(GETHOSTBY_FUNC);
    }

    return new_host;
}

/**
 * Blocks the process & waiting for client connection
 * @param readFDSet set of fds to read from
 */
void waitForClientConnection(int fd_range, fd_set &readFDSet){
    int return_val = select(fd_range, &readFDSet, NULL,NULL,NULL);
    if (return_val < 0){
        report_error(SELECT_FUNC);
    }
}

/**
 * checks weather the fd number clientFd is
 * a valid fd number.
 * @param clientFd
 */
void validate_fd(int clientFd){
    if(clientFd < 0){
        report_error(ACCEPT_FUNC);
    }
}

/**
 * Checks weather we need to update the range of fd.
 * return the new range accordingly.
 * @param fd_range
 * @param clientFD
 */
int validate_and_update_range(int fd_range, int clientFD){
    if(clientFD >= fd_range ){
        return clientFD +1;
    }

    return clientFD;
}

/**
 * checks weather user_input is an EXIT message.
 * @param user_input
 * @return
 */
bool is_exit_msg(string user_input){
    return (user_input.compare("EXIT") == 0);
}

/**
 * Sends to the clients_fds[i] an exit message
 * @param i - an index
 */
void send_user_exit_message(int i){
    string msg = "exit\n";
    if (send(clients_fds[i],msg.c_str(), msg.length(),0) == -1){
        report_error(SEND_FUNC);
    }
}

/**
 * Free memory we allocated for clients_fds
 */
void free_client_fds(){
    unsigned int j = 0;
    for (j = 0; j < clients_fds.size(); ++j) {
        send_user_exit_message(j);
    }
    clients_fds.clear();
}

/**
 * Free the server's memory.
 */
void free_memory(){

    free_client_fds();

    clients_per_fds.clear();
    groups_per_clients.clear();

    FD_ZERO(&clientFDSet);

    close(server_socket_fd);

}

/**
 *  EXIT command was typed.
 *  Shuts down the server, and free memory.
 */
void exit_server(){
    free_memory();
    print_exit();
    close(server_socket_fd);
    exit(0);
}

/**
 *removes the element client_fd from the client_fds vector
 */
void find_and_remove_fd(int clients_fd){
    vector<int>::iterator it = find(clients_fds.begin(), clients_fds.end(), clients_fd);
    clients_fds.erase(it);
}

/**
 * removes the client_fd FD from all server sources
 */
void remove_fd(int clients_fd){
    FD_CLR(clients_fd, &clientFDSet);
    find_and_remove_fd(clients_fd);
}

bool socket_closed_finished_reading(int bytes_was_read){
    if(bytes_was_read == 0){ return true; }
    else{return false;}
}

/**
 * Receives client input by the buffer, from the fd client fd.
 * @param clients_fd
 * @param input_buffer
 */
void receive_client_input(int clients_fd, char *input_buffer){
    ssize_t bytes_counter = 0;
    int bytes_was_read = 0;
    char last_letter;
    char *buffer_pointer;

    last_letter = '\0';
    buffer_pointer = input_buffer;

    while(!(last_letter == '\n')){
        bytes_was_read = (int) recv (clients_fd, buffer_pointer, (WA_MAX_MESSAGE - bytes_counter), 0);
        if (bytes_was_read > 0) {
            bytes_counter += bytes_was_read;
            buffer_pointer += bytes_was_read;
        }
        if (bytes_was_read < 1) {
            report_error(RECV_FUNC);

            if(socket_closed_finished_reading(bytes_was_read)){ // Finished reading - socket was closed
                remove_fd(clients_fd);
            }
            return;
        }
        int to_buffer = bytes_counter-1;
        last_letter = input_buffer[to_buffer];
    }
}

/**
 *  handles the input we got from the standard server input.
 *  such as: "EXIT" etc..
 */
void handleServerInput(){
    string user_input;
    getline(cin, user_input);

    if (is_exit_msg(user_input))
        exit_server();
}

/**
 * hanldes new connection request from a client who wants to connect the server.
 * @param clientFd
 * @param clientSocket
 * @param fd_range
 * @return the new fd_range in order to keep supplying the lowest available fd-number for
 * a new client who wants to connect the server.
 */
int handleNewClient(int clientFd, sockaddr_in clientSocket, int fd_range){

    int size = sizeof(struct sockaddr_in);
    clientFd = accept(server_socket_fd, (struct sockaddr*)&clientSocket, (socklen_t*)&size);
    validate_fd(clientFd);
    fd_range = validate_and_update_range(fd_range, clientFd); // validate & update the range of fd files

    FD_SET(clientFd, &clientFDSet); // enabling the byte for future reading - we must read the name or the client
    clients_fds.push_back(clientFd);

    return fd_range;
}

/**
 * handles new message, arrived from existing client.
 * @param readFDSet
 */
void handleClientMSG(fd_set readFDSet){
    for (unsigned int index = 0; index < clients_fds.size(); ++index)
        if(FD_ISSET(clients_fds[index], &readFDSet))
        {
            char input_buffer[WA_MAX_MESSAGE] = {0}; // we could use memset instead
            receive_client_input(clients_fds[index], input_buffer);
            handle_client_input(clients_fds[index] , input_buffer);
        }
}

/**
 * takes care of accepting connections from clients,
 */
void start_accept_client_connections() {

    fd_set readFDSet;
    int clientFd;

    FD_ZERO(&readFDSet);

    FD_ZERO(&clientFDSet);
    FD_SET(server_socket_fd, &clientFDSet);
    FD_SET(STDIN_FILENO, &clientFDSet);

    int fd_range = server_socket_fd + 1;
    sockaddr_in clientSocket;
    while(true){

        readFDSet = clientFDSet;
        waitForClientConnection(fd_range, readFDSet);

        if (FD_ISSET(STDIN_FILENO, &readFDSet))// input from standard server input
        {
            handleServerInput();
        }
        else if(FD_ISSET(server_socket_fd, &readFDSet))// we've got message from new clint who wants to connect the server
        {
            fd_range = handleNewClient(clientFd, clientSocket, fd_range);
        }
        else // handling client request - one of the existing clients have sent a new request
        {
            handleClientMSG(readFDSet);
        }
    }

}

/**
 * creates & initializes the server's socket
 * initialize the server_socket_fd as the server's fd.
 * initializing it as localhost.
 * @param port - is the server's port number
 */
void initialize_socket(char *port){

    struct hostent* host;
    struct sockaddr_in server_socket;
    int server_fd;
    int int_port = atoi(port);

    host = get_localhost();  // sets the host
    memset((char*)&server_socket, 0, sizeof(struct sockaddr_in)); // sets sin_zero
    server_socket.sin_family = host->h_addrtype; // sets the connection's type
    memcpy(&server_socket.sin_addr, host->h_addr, host->h_length); // sets the ip to be the "host"'s ip
    server_socket.sin_port = htons(int_port); // sets the port

    //................................... creating the socket's fd .........................//
    server_fd = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (server_fd < 0){
        report_error(SOCKET_FUNC);
    }
    server_socket.sin_addr.s_addr = INADDR_ANY;

    // Initializing the socket for the fd so it will listen to:
    if (bind(server_fd, ((struct sockaddr *)&server_socket), sizeof(struct sockaddr_in)) < 0){
        report_error(BIND_FUNC);
        close(server_fd);
    }

    // Sets max number of pending requests for the socket_fd
    if (listen(server_fd, WA_MAX_PEND) == -1 ){
        report_error(LISTEN_FUNC);
    }

    server_socket_fd = server_fd;
}

int main(int numOfArgs, char *args[]) {

    char *port = args[1];

    args_validation_check(numOfArgs);
    initialize_socket(port);
    start_accept_client_connections();
    close(server_socket_fd);

    return 0;
}
