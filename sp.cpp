#include<filesystem>

#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include <signal.h>

#include <cstdlib>
#include <netdb.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <openssl/err.h>
#include <sys/file.h>
#include <fcntl.h>
#include <vector>

#define PORT 8080
#define BUFFER_SIZE 8192
bool http = true;

using namespace std;

bool do_log(string data){

    vector<string> http_methods = {"GET","POST","PUT","DELETE","HEAD","OPTIONS","PATCH","CONNECT","TRACE"}; 
    for(string s:http_methods){
        if(data.starts_with(s)){
            //if(data.find("login") != std::string::npos){
                return true;
            //}
        }
    }
    return false;


}

void logdata(ofstream& file, char* buffer, int r) {
    file.write(buffer, r);
    file.flush();
}
struct  hostinfo{
    hostent* serv;
    string host;
};

hostinfo get_serv_ip(string & req){
    size_t pos = req.find("Host:");

    if (pos == string::npos)
        return {};

    pos += 5;

    while (req[pos] == ' ')
        pos++;
    
    size_t end = req.find("\r\n", pos);

    string url = req.substr(pos, end - pos);
    end=url.find(":");
    http=true;
    if (end!= string::npos){

        //https
        http=false;
        url = url.substr(0, end);
    }
    
    hostent* serv = gethostbyname(url.c_str());
    
    if (!(serv&&
        serv->h_addr_list&&
        serv->h_addr_list[0])) {
            cerr << "DNS lookup failed\n";
            return {nullptr,nullptr};
    }
    return {serv, url};;
}

int main(){
    /*int inc_fd[2];
    pipe(inc_fd);
    int inc=0;
    int *inc_ptr=&inc;
    write(inc_fd[1],inc_ptr,sizeof(int));*/

    unordered_map<string,string> cert_cache;
    int server_fd=socket(AF_INET, SOCK_STREAM,0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);


    addr.sin_addr.s_addr=INADDR_ANY;
    
    bind(server_fd,(sockaddr*)&addr,sizeof(addr));
    listen(server_fd,10);
    ofstream file("packets.txt");
    

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    int lock_fd = open("/tmp/certgen.lock", O_CREAT | O_RDWR, 0666);
    while(true){
        
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
            string req(buffer);
            
            string serv_ip;
            string host;
            if (bytes>0){
                //if (req.find("login")!=string::npos)
                
                hostinfo hi=get_serv_ip(req);
                hostent* serv=hi.serv;
                
                if (serv){

                    serv_dup=serv;
                    struct in_addr ip;

                    memcpy(&ip, serv->h_addr_list[0], sizeof(struct in_addr));

                    
                    serv_ip=inet_ntoa(ip);
                    host=hi.host;

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
                
                SSL_set_tlsext_host_name(remote_ssl, host.c_str());

                int ret = SSL_connect(remote_ssl);

                if (ret <= 0) {
                    int err = SSL_get_error(remote_ssl, ret);

                    fprintf(stderr, "SSL_connect failed\n");
                    fprintf(stderr, "SSL_get_error = %d\n", err);

                    ERR_print_errors_fp(stderr);

                    exit(1);
                }
                            
                if(http){

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
                    
                    string crt;
                    
                    if (cert_cache.find(host) == cert_cache.end()) {

                        flock(lock_fd, LOCK_EX);

                        // double-check after locking (important)
                        if (cert_cache.find(host) == cert_cache.end()) {

                            string id = host; 

                            string cnf = "/tmp/" + id + ".cnf";
                            string csr = "/tmp/" + id + ".csr";
                            crt = "/tmp/" + id + ".crt";

                            ofstream san(cnf);

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
                            "basicConstraints=critical,CA:FALSE\n"
                            "keyUsage=critical,digitalSignature,keyEncipherment\n"
                            "extendedKeyUsage=serverAuth\n"
                            "subjectAltName=@alt_names\n"
                            "\n"
                            "[alt_names]\n"
                            "DNS.1=" << host << "\n";

                            san.close();

                            string cmd1 =
                                "openssl req -new "
                                "-key CA/example_nopass.key "
                                "-out " + csr +
                                " -config " + cnf;

                            string cmd2 =
                                "openssl x509 -req "
                                "-in " + csr +
                                " -CA CA/rootCA.pem "
                                "-CAkey CA/rootCA_nopass.key "
                                "-CAcreateserial "
                                "-out " + crt +
                                " -days 365 "
                                "-sha256 "
                                "-extfile " + cnf +
                                " -extensions v3_req";

                            if (system(cmd1.c_str()) != 0) {
                                fprintf(stderr, "cmd1 failed\n");
                            }

                            if (system(cmd2.c_str()) != 0) {
                                fprintf(stderr, "cmd2 failed\n");
                            }

                            cert_cache[host] = crt;
                        }

                        flock(lock_fd, LOCK_UN);
                    }
                    else {
                         crt = cert_cache[host];
                    }


                    system(("openssl x509 -in " + crt + " -subject -noout").c_str());
                    SSL_CTX_use_certificate_file(
                        server_ctx,
                        crt.c_str(),
                        SSL_FILETYPE_PEM
                    );

                    SSL_CTX_use_PrivateKey_file(
                        server_ctx,
                        "CA/example_nopass.key",
                        SSL_FILETYPE_PEM
                    );

                    SSL* client_ssl = SSL_new(server_ctx);

                    SSL_set_fd(client_ssl, client);

                    if (!SSL_accept(client_ssl))
                    {
                        ERR_print_errors_fp(stderr);
                    }
                    
                    while (true) {

                        FD_ZERO(&fds);

                        FD_SET(client, &fds);
                        FD_SET(remote_fd, &fds);

                        int maxfd = max(client, remote_fd);

                        int activity = select(maxfd + 1,&fds,nullptr,nullptr,nullptr);

                        if (activity < 0)
                            break;

                        // browser -> server
                        if (FD_ISSET(client, &fds)) {
                            
                            int r = SSL_read(client_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;
                            string fname="packets/"+to_string(getpid());
                            ofstream file1(fname, ios::binary);
                            file1.write(buffer, r);
                            file1.close();
                            /*read(inc_fd[0],inc_ptr,sizeof(int));
                            (*inc_ptr)++;
                            cout<<*inc_ptr;
                            write(inc_fd[1],inc_ptr,sizeof(int));*/

                            
                        
                            raise(SIGSTOP);

                            ifstream file2(fname, ios::binary);

                            std::string packet(
                                (std::istreambuf_iterator<char>(file2)),
                                std::istreambuf_iterator<char>()
                            );
                            SSL_write(remote_ssl, packet.data(), r);
                        }


                        // server -> browser
                        if (FD_ISSET(remote_fd, &fds)) {

                            int r = SSL_read(remote_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;

                            //logdata(file, buffer, r);

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
