#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include <signal.h>

#include <sys/un.h>

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

#include <chrono>
#include <mutex>
#include <sys/wait.h>

int PORT=8080;
#define BUFFER_SIZE 8192
bool http = true;


volatile bool running = true;

void stop_handler(int) {
    running = false;
}

using namespace std;

mutex logMutex;
string LOG_PATH = "../backend/ingest/proxy_events.log";

string jsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}


void reap_children(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}



void logRequest(
    const string& sessionId,
    const string& method,
    const string& host,
    const string& path,
    int statusCode,
    int requestSize,
    int responseSize,
    bool tls,
    bool flagged,
    const string& flaggedReason = ""
) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::ofstream out(LOG_PATH, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Failed to open log file: " << LOG_PATH << "\n";
        return;
    }
    out << "{"
        << "\"session_id\":\"" << jsonEscape(sessionId) << "\","
        << "\"method\":\"" << jsonEscape(method) << "\","
        << "\"host\":\"" << jsonEscape(host) << "\","
        << "\"path\":\"" << jsonEscape(path) << "\","
        << "\"status_code\":" << statusCode << ","
        << "\"request_size\":" << requestSize << ","
        << "\"response_size\":" << responseSize << ","
        << "\"tls\":" << (tls ? "true" : "false") << ","
        << "\"flagged\":" << (flagged ? "true" : "false") << ","
        << "\"flagged_reason\":" << (flaggedReason.empty() ? "null" : "\"" + jsonEscape(flaggedReason) + "\"")
        << "}\n";

    out.flush(); // important: flush so the ingest worker's fs.watch sees it promptly
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
    return {serv, url};
}

   
int main(int argc, char* argv[]) {
    bool flaged=false;
    string flagedReason="";
    signal(SIGCHLD, reap_children);
    if (argc > 1) {
        try {
            size_t pos;
            PORT = std::stoi(argv[1], &pos);


            if (pos != std::strlen(argv[1])) {
                throw std::invalid_argument("extra characters");
            }

            if ( PORT < 0 || PORT > 65535) {
                std::cout << "Enter a valid port number (0-65535)\n";
                return 1;
            }
        } catch (...) {
            std::cout << "Usage: " << argv[0] << " [port number]\n";
            return 1;
        }
    }
    string sessionId;
    if (argc > 2) {
        sessionId = argv[2];
    }
 
    
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
    setpgid(0, 0);
    signal(SIGTERM, stop_handler);
    while(running){
        
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

            string method = "UNKNOWN";
            string reqPath = "/";
            {
                size_t sp1 = req.find(' ');
                size_t sp2 = req.find(' ', sp1 + 1);
                if (sp1 != string::npos && sp2 != string::npos) {
                    method = req.substr(0, sp1);
                    reqPath = req.substr(sp1 + 1, sp2 - sp1 - 1);
                }
            }
            long requestSize = bytes;
            long responseSize = 0;
            int statusCode = 0;
            
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
                    logRequest(sessionId, method, host, reqPath, statusCode, requestSize, responseSize, !http, flaged, flagedReason);

                    send(remote_fd,buffer,bytes,0);
                    
                    while (true) {
                        int r = recv(remote_fd, buffer, BUFFER_SIZE, 0);
                        if (r <= 0)
                            break;
                        if (statusCode == 0 && r > 12) {
                            sscanf(buffer, "HTTP/%*d.%*d %d", &statusCode);
                        }
                        responseSize += r;
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
                        if (!running)
                            break;

                        FD_ZERO(&fds);

                        FD_SET(client, &fds);
                        FD_SET(remote_fd, &fds);

                        int maxfd = max(client, remote_fd);
                        struct timeval tv;
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;
                        int activity=select(maxfd + 1, &fds, nullptr, nullptr, &tv);

                        if (activity < 0)
                            break;
                        logRequest(sessionId, method, host, reqPath, statusCode, requestSize, responseSize, !http, flaged, flagedReason);

                        // browser -> server
                        if (FD_ISSET(client, &fds)) {
                            
                            int r = SSL_read(client_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;

                            requestSize += r;

                            int edit_sock = socket(AF_UNIX, SOCK_STREAM, 0);
                            sockaddr_un editor_addr{};
                            editor_addr.sun_family = AF_UNIX;
                            strncpy(editor_addr.sun_path, "/tmp/mitm_editor.sock", sizeof(editor_addr.sun_path) - 1);

                            if (connect(edit_sock, (sockaddr*)&editor_addr, sizeof(editor_addr)) < 0) {

                                SSL_write(remote_ssl, buffer, r);
                                close(edit_sock);
                            } else {
                                uint32_t len_be = htonl((uint32_t)r);
                                write(edit_sock, &len_be, 4);
                                write(edit_sock, buffer, r);

                                uint32_t rlen_be;
                                if (read(edit_sock, &rlen_be, 4) == 4) {
                                    uint32_t rlen = ntohl(rlen_be);
                                    std::string packet(rlen, '\0');
                                    size_t got = 0;
                                    while (got < rlen) {
                                        ssize_t n = read(edit_sock, &packet[got], rlen - got);
                                        if (n <= 0) break;
                                        got += n;
                                    }

                                    SSL_write(remote_ssl, packet.data(), got);
                                }
                                close(edit_sock);
                            }
                        }

                        // server -> browser
                        if (FD_ISSET(remote_fd, &fds)) {

                            int r = SSL_read(remote_ssl, buffer, BUFFER_SIZE);

                            if (r <= 0)
                                break;

                            if (statusCode == 0 && r > 12) {
                                sscanf(buffer, "HTTP/%*d.%*d %d", &statusCode);
                            }
                            responseSize += r;


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
