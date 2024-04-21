#include <iostream>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 3001
#define BUFFER_SIZE 1024

class request {
    public:
        std::string req;
        std::string path;
        std::string method;
        std::string type;
};

std::string read_file(std::string path1) {
    std::string path = path1;
    std::string text;
    std::string line;
    std::ifstream File(path);
    if(File.is_open()) {
        while(getline(File, line)) {
            text += line;
        }
        File.close();
    } else {
        return "404";
    }

    return text;
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();

    ctx = SSL_CTX_new(method);
    if(!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    if(SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if(SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    SSL_CTX *ctx;
    ctx = create_context();
    configure_context(ctx);

    struct sockaddr_in server_info = {0};
    struct sockaddr client_info = {0};

    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);

    socklen_t server_info_len = sizeof(server_info);
    socklen_t client_info_len = sizeof(client_info);

    // create socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > sfd) {
        std::cerr << "Socket creation failed.\n";
        exit(-1);
    }

    // bind
    if(0 > bind(sfd, (struct sockaddr*)&server_info, server_info_len)) {
        std::cerr << "Bind failed.\nTry changing the port.\n";
        exit(-1);
    }

    // listen
    if(0 > listen(sfd, 0)) {
        std::cerr << "Cannot listen.\n";
        exit(-1);
    }

    while(true) {
        SSL *ssl;

        // accept
        int cfd = accept(sfd, &client_info, &client_info_len);
        if(0 > cfd) {
            std::cerr << "Failed to accept.\n";
            exit(-1);
        }

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, cfd);

        if(SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            request req;
            char buffer[BUFFER_SIZE];
            int received = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
            if(received > 0) {
                buffer[received] = '\0';

                req.req = buffer;
                req.method = req.req.substr(0, req.req.find(' '));
                req.path = req.req.substr(req.req.find(' ')+1, req.req.find("HTTP/1.1"));
                req.path = req.path.substr(0, req.path.length()-5);
                if(req.path[req.path.length()-1] == '/') {
                    req.path += "index.html";
                }

                if(req.path.length() > 5) {
                    if(req.path.substr(req.path.length()-5, 5) == ".html") req.type = "text/html";
                    else if(req.path.substr(req.path.length()-4, 4) == ".css") req.type = "text/css";
                    else if(req.path.substr(req.path.length()-3, 3) == ".js") req.type = "application/js";
                    else req.type = "text/plain";
                } else req.type = "text/plain";

                std::cout << req.method << "\t" << req.path << "\n";
            } else {
                ERR_print_errors_fp(stderr);
            }

            std::string file = read_file("./public"+req.path);
            if(file == "404") {
                std::string text = "404 Not Found";
                std::string response = "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: " + req.type + "\r\n"
                                       "Content-Length: " + std::to_string(text.length()) + "\r\n\r\n"
                                       + text;
                SSL_write(ssl, response.c_str(), strlen(response.c_str()));
            } else {
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: " + req.type + "\r\n"
                                       "Content-Length: " + std::to_string(file.length()) + "\r\n\r\n"
                                       + file;

                SSL_write(ssl, response.c_str(), strlen(response.c_str()));
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(cfd);
    }

    return EXIT_SUCCESS;
}