#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <thread>
#include <fstream>
#include <zlib.h>

std::string gzip(const std::string &data) {
    z_stream zs{};
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED,
                 15 + 16, 8, Z_DEFAULT_STRATEGY);

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = data.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        outstring.append(outbuffer, sizeof(outbuffer) - zs.avail_out);
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return outstring;
}

// Reads exactly one full HTTP request (headers + body) off the socket,
// looping recv() as many times as needed. Returns false if the peer
// closed the connection before a full request arrived.
bool read_full_request(int client_fd, std::string &out_request, std::string &out_body) {
    char buffer[4096];
    std::string data;

    size_t header_end = std::string::npos;
    while (header_end == std::string::npos) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) return false;          // connection closed / error
        data.append(buffer, bytes);            // binary-safe append, no null-term issue
        header_end = data.find("\r\n\r\n");
        if (data.size() > (1 << 20)) return false; // guard against runaway headers
    }

    // Parse Content-Length from what we have so far
    long content_length = 0;
    size_t pos = data.find("Content-Length:");
    if (pos != std::string::npos && pos < header_end) {
        size_t end = data.find("\r\n", pos);
        std::string len_str = data.substr(pos + 16, end - (pos + 16));
        try { content_length = std::stol(len_str); } catch (...) { content_length = 0; }
    }

    size_t body_start = header_end + 4;
    size_t body_have = data.size() - body_start;

    // Keep reading until we actually have the full body
    while ((long)body_have < content_length) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) return false;
        data.append(buffer, bytes);
        body_have = data.size() - body_start;
    }

    out_request = data.substr(0, body_start); // headers (+ terminator)
    out_body = data.substr(body_start, content_length > 0 ? content_length : (data.size() - body_start));
    return true;
}

void handle_client(int client_fd, std::string directory) {
    while (true) {
        std::string request, body;
        if (!read_full_request(client_fd, request, body)) break;

        std::istringstream request_stream(request);
        std::string method, path, version;
        request_stream >> method >> path >> version;

        std::string user_agent, line;
        bool gzip_supported = false;
        bool connection_close = false;

        while (std::getline(request_stream, line)) {
            if (line.rfind("User-Agent:", 0) == 0) {
                user_agent = line.substr(12);
                if (!user_agent.empty() && user_agent.back() == '\r') user_agent.pop_back();
            }
            if (line.rfind("Accept-Encoding:", 0) == 0 && line.find("gzip") != std::string::npos) {
                gzip_supported = true;
            }
            if (line.rfind("Connection:", 0) == 0 && line.find("close") != std::string::npos) {
                connection_close = true;
            }
        }

        std::string response;

        if (path == "/") {
            response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        } else if (path.rfind("/echo/", 0) == 0) {
            std::string msg = path.substr(6);
            if (gzip_supported) {
                std::string compressed = gzip(msg);
                response = "HTTP/1.1 200 OK\r\n"
                    "Content-Encoding: gzip\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(compressed.size()) + "\r\n\r\n" + compressed;
            } else {
                response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(msg.size()) + "\r\n\r\n" + msg;
            }
        } else if (path == "/user-agent") {
            response = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " + std::to_string(user_agent.size()) + "\r\n\r\n" + user_agent;
        } else if (path.rfind("/files/", 0) == 0) {
            std::string filename = path.substr(7);

            if (method == "GET") {
                std::ifstream file(directory + "/" + filename, std::ios::binary);
                if (!file.is_open()) {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                } else {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    std::string file_body = ss.str();
                    response = "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: " + std::to_string(file_body.size()) + "\r\n\r\n" + file_body;
                }
            } else if (method == "POST") {
                std::ofstream file(directory + "/" + filename, std::ios::binary);
                file.write(body.data(), body.size()); // full body, not truncated
                file.close();
                response = "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n";
            }
        } else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        }

        if (connection_close) {
            response.insert(response.find("\r\n") + 2, "Connection: close\r\n");
        }

        send(client_fd, response.c_str(), response.size(), 0);
        if (connection_close) break;
    }
    close(client_fd);
}

int main(int argc, char **argv) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { std::cerr << "Failed to create server socket\n"; return 1; }

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n"; return 1;
    }

    int connection_backlog = 128; // raised from 5
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n"; return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    std::string directory = "";
    if (argc >= 3) directory = argv[2];

    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) { std::cerr << "accept failed\n"; continue; }
        std::thread(handle_client, client_fd, directory).detach();
    }
    return 0;
}
