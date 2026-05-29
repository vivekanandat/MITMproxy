#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netdb.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <openssl/err.h>


#define PORT 8080
#define BUFFER_SIZE 8192
bool http = true;

using std::cout;
using std::flush;

void logdata(std::ofstream& file, char* buffer, int r) {

    std::string data(buffer, r);

    bool is_http =
        data.starts_with("GET")  ||
        data.starts_with("POST") ||
        data.starts_with("PUT")  ||
        data.starts_with("DELETE") ||
        data.starts_with("HTTP/");

    if (!is_http)
        return;

    size_t end = data.find("\r\n\r\n");

    if (end == std::string::npos)
        return;

    file << data.substr(0, end);

    file << "\n------------------------------------------------------------------------------\n";
}


hostent* get_serv_ip(std::string & req){
    size_t pos = req.find("Host:");

    if (pos == std::string::npos)
        return {};

    pos += 5;

    while (req[pos] == ' ')
        pos++;
    
    size_t end = req.find("\r\n", pos);

    std::string url = req.substr(pos, end - pos);
    end=url.find(":");
    http=true;
    if (end!= std::string::npos){

        //https
        http=false;
        url = url.substr(0, end);
    }
    
    hostent* serv = gethostbyname(url.c_str());

    
    if (!(serv&&
        serv->h_addr_list&&
        serv->h_addr_list[0])) {
            std::cerr << "DNS lookup failed\n";
            return nullptr;
    }
    return serv;
}

int main(){
    int server_fd=socket(AF_INET, SOCK_STREAM,0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);


    addr.sin_addr.s_addr=INADDR_ANY;
    
    bind(server_fd,(sockaddr*)&addr,sizeof(addr));
    listen(server_fd,10);
    std::ofstream file("packets.txt");
    
    cout << PORT << "\n";

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    while(true){
        cout<<"\n."<<flush;
        
        int client = accept(server_fd,nullptr,nullptr);

        if (client < 0) {
            perror("accept");
            exit(0);
        }
        
        
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);

            char buffer[BUFFER_SIZE];
            memset(buffer, 0,BUFFER_SIZE);
            hostent* serv_dup={};
            int bytes=recv(client,buffer,BUFFER_SIZE-1,0);
            std::string req(buffer);
            
            std::string serv_ip;
            std::string host;
            if (bytes>0){
                //if (req.find("login")!=std::string::npos)

                hostent* serv=get_serv_ip(req);
                
                if (serv){

                    serv_dup=serv;
                    struct in_addr ip;

                    memcpy(&ip, serv->h_addr_list[0], sizeof(struct in_addr));

                    
                    serv_ip=inet_ntoa(ip);
                    host=serv->h_name;

                }else{
                    exit(0);
                }            
            }

            int remote_fd=socket(AF_INET, SOCK_STREAM,0);
            int opt = 1;
            setsockopt(remote_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            sockaddr_in remote_addr{};
            remote_addr.sin_family = AF_INET;
            remote_addr.sin_port = htons(80);

            memcpy(
                &remote_addr.sin_addr,
                serv_dup->h_addr_list[0],
                serv_dup->h_length
            );
            
            if(!http){
                remote_addr.sin_port = htons(443);
            }

            int check=connect(remote_fd,(sockaddr*)&remote_addr,sizeof(sockaddr_in));
            if(check==0){
                SSL_CTX* remote_ctx = SSL_CTX_new(TLS_client_method());

                SSL* remote_ssl = SSL_new(remote_ctx);

                SSL_set_fd(remote_ssl, remote_fd);
                
                SSL_set_tlsext_host_name(remote_ssl, serv_dup->h_name);

                if (SSL_connect(remote_ssl) <= 0) {
                    ERR_print_errors_fp(stderr);
                    exit(1);
                }
                cout<<"\n"<<serv_dup->h_name<<serv_ip<<"\n"<<flush;
            
                if(http){
                    cout<<"conn\n"<<flush;

                    send(remote_fd,buffer,bytes,0);
                    
                    while (true) {
                        int r = recv(remote_fd, buffer, BUFFER_SIZE, 0);
                        if (r <= 0)
                            break;
                        send(client, buffer, r, 0);
                    }
                }
                else{
                    fd_set fds;
                    
			        const char* resp = "HTTP/1.1 200 Connection Established\r\n\r\n";
                    send(client, resp, strlen(resp), 0);
                    SSL_CTX* server_ctx = SSL_CTX_new(TLS_server_method());

                    std::ofstream san("san.cnf");

                    san <<
                    "[req]\n"
                    "distinguished_name=req_distinguished_name\n"
                    "req_extensions=v3_req\n"
                    "prompt=no\n"
                    "\n"
                    "[req_distinguished_name]\n"
                    "CN=" << host << "\n"
                    "\n"
                    "[v3_req]\n"
                    "subjectAltName=@alt_names\n"
                    "\n"
                    "[alt_names]\n"
                    "DNS.1=" << host << "\n";

                    san.close();
                    std::string cmd1 =
                    "openssl req -new "
                    "-key CA/example_nopass.key "
                    "-out temp.csr "
                    "-config san.cnf";

                    system(cmd1.c_str());

                    std::string cmd2 =
                    "openssl x509 -req "
                    "-in temp.csr "
                    "-CA CA/rootCA.pem "
                    "-CAkey CA/rootCA_nopass.key "
                    "-CAcreateserial "
                    "-out temp.crt "
                    "-days 365 "
                    "-sha256 "
                    "-extfile san.cnf "
                    "-extensions v3_req";

                    system(cmd2.c_str());
                    SSL_CTX_use_certificate_file(
                        server_ctx,
                        "temp.crt",
                        SSL_FILETYPE_PEM
                    );

                    SSL_CTX_use_PrivateKey_file(
                        server_ctx,
                        "CA/example_nopass.key",
                        SSL_FILETYPE_PEM
                    );

                    SSL* client_ssl = SSL_new(server_ctx);

                    SSL_set_fd(client_ssl, client);

                    if (SSL_accept(client_ssl) <= 0) {
                        ERR_print_errors_fp(stderr);
                        exit(1);
                    }
                    while (true) {

                        FD_ZERO(&fds);

                        FD_SET(client, &fds);
                        FD_SET(remote_fd, &fds);

                        int maxfd = std::max(client, remote_fd);

                        int activity = select(maxfd + 1,&fds,nullptr,nullptr,nullptr);

                        if (activity < 0)
                            break;

                        // browser -> server
                        if (FD_ISSET(client, &fds)) {

                            int r = SSL_read(client_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;

                            logdata(file, buffer, r);

                            SSL_write(remote_ssl, buffer, r);
                            }


                        // server -> browser
                        if (FD_ISSET(remote_fd, &fds)) {

                            int r = SSL_read(remote_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;

                            logdata(file, buffer, r);

                            SSL_write(client_ssl, buffer, r);
                        }
                    }
                } 
            }
            else if(check < 0) {
                perror("connect");
                exit(1);
            }

            close(client);

            file.close();
            close(server_fd);
            exit(0);

        } else if (pid > 0) {
            // Parent process
            close(client); // parent doesn't need client socket
        } else {
            perror("fork");
        }


    }

    file.close();
    close(server_fd);
}
