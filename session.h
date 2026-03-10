#ifndef SESSION_H
#define SESSION_H
#include "net.h"
#include "fstream"
#include "boost/uuid.hpp"
#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"


class Server;


class Session : public std::enable_shared_from_this<Session>
{
    std::fstream file_stream;
    Server& server;
    asio::ssl::stream<tcp::socket> ssl_stream;
    beast::flat_buffer buffer;
    http::request<http::string_body> req;
    std::vector<char> buff;
    std::shared_ptr<asio::steady_timer> stream_timer;
    void do_read();
    void do_close();

    void start_streaming();

    void handle_api();

    void stream_loop();

    void open_file_read();
    std::streamsize  read_file_chunk();

    boost::uuids::uuid generate_uuid();

    std::pair<bool, std::string> verify_jwt(const std::string& token);

    std::pair<bool, minio::s3::BucketExistsResponse> check_bucket(const std::string& bucket);

public:
    Session(Server& server, std::shared_ptr<ip::tcp::socket> socket, asio::ssl::context& contx);
    void run();
};

#endif // SESSION_H
