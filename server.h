#ifndef Server_H
#define Server_H
#include "net.h"
#include "mutex"
#include "session.h"
#include <forward_list>
#include <fstream>
#include "database.h"


class Server : public std::enable_shared_from_this<Server>
{

    asio::io_context ioc;
    ip::tcp::acceptor acceptor;
    std::mutex mtx;
    std::forward_list<std::shared_ptr<Session>> sessions;
    ssl::context ctx;


    std::unique_ptr<minio::creds::Provider> s3Provider;

    void s3CLientInit();

    void start_acceptor();
    std::string load_secret();

public:
    DataBase db;
    std::unique_ptr<minio::s3::Client> s3Client;
    const std::string secret;
    void start();
    void load_server_certificate(asio::ssl::context& contx);
    Server();
};

#endif // Server_H
