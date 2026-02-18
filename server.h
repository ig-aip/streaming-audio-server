#ifndef Server_H
#define Server_H
#include "net.h"
#include "mutex"
#include "session.h"
#include <forward_list>

class Server : public std::enable_shared_from_this<Server>
{

    asio::io_context ioc;
    ip::tcp::acceptor acceptor;
    std::mutex mtx;
    std::forward_list<std::shared_ptr<Session>> sessions;
    ssl::context ctx;

    void start_acceptor();

public:
    void start();
    void load_server_certificate(asio::ssl::context& contx);
    Server();
};

#endif // Server_H
