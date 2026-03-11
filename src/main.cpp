#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <thread>
#include <fstream>

void handle_client(int client_fd , std::string directory){
  char buffer[1024] = {0};
ssize_t bytes= recv(client_fd, buffer, sizeof(buffer), 0);

if(bytes <= 0){
  close(client_fd);
  return;
}
std::string request(buffer);
std::istringstream request_stream(request);

std::string method;
std::string path;
std::string version;

request_stream >> method >> path >> version;

std::cout << "Requested path: " << path << std::endl;

std::string response;

std::string user_agent;
std::string line;

while(std::getline(request_stream,line)){
  if(line.substr(0,11) == "User-Agent:"){
    user_agent = line.substr(12);

    if(!user_agent.empty() && user_agent.back()=='\r'){
      user_agent.pop_back();
    }
    break;
  }
}

if (path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
} else if(path.size() >= 6 && path.substr(0,6) == "/echo/"){
    std::string msg = path.substr(6);

    response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: " + std::to_string(msg.size()) + "\r\n\r\n" + msg;
} else if(path == "/user-agent"){
    response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: " + std::to_string(user_agent.size()) + "\r\n\r\n" + user_agent;
    }  else if(path.size()>=7 && path.substr(0,7)=="/files/"){
      std::string filename = path.substr(7);
      std::ifstream file(directory + "/" +filename);
      if(!file.is_open()){
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
      } else {
      std::stringstream buffer;
      buffer<<file.rdbuf(); // rdbuf = raw data buffer
      std::string body = buffer.str();
      
      response = 
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/octet-stream\r\n"
      "Content-Length: "+std::to_string(body.size()) + "\r\n\r\n" + body;}
      } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
} 

send(client_fd, response.c_str(), response.size(), 0);

  close(client_fd);
  
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // TODO: Uncomment the code below to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
 std::string directory = "";
 if(argc >=3){
  directory = argv[2];
 }

  while(true){

  int client_fd = accept(server_fd, 
    (struct sockaddr *) &client_addr, 
    (socklen_t *) &client_addr_len);

  if (client_fd < 0) {
      std::cerr << "accept failed\n";
      continue;
  }

  std::thread t(handle_client,client_fd,directory);
  t.detach();
  }
  return 0;
}
