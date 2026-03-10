#include "session.h"
#include "server.h"
#include "iostream"



Session::Session(Server& server, std::shared_ptr<ip::tcp::socket> socket, asio::ssl::context& contx) :
    server(server),
    ssl_stream(std::move(*socket), contx),
    buff(4096)
{
    stream_timer = std::make_shared<asio::steady_timer>(ssl_stream.get_executor());
}

void Session::run()
{
    auto self = shared_from_this();
    ssl_stream.async_handshake(ssl::stream_base::server,
                               [self](boost::system::error_code er){
                                   if(!er){
                                       self->do_read();
                                   }else{
                                       std::cerr << "error in handshake: " << er.what() << std::endl;
                                   }
                               });
}

void Session::do_read()
{
    req = {};
    auto self = shared_from_this();
    http::async_read(ssl_stream,
                     buffer,
                     req,
                     [self](beast::error_code er, size_t bytes){
                         boost::ignore_unused(bytes);
                         if(!er){
                            self->handle_api();
                         }
                         else if(er == http::error::end_of_stream || er == asio::ssl::error::stream_truncated){ self->do_close(); }
                         else{
                            std::cerr << "error in asyc read: " << er.what() << std::endl;
                         }
                     });
}

inline void to_json(nlohmann::json& j, const music_file& m){
    j = nlohmann::json{
        {"id", m.id},
        {"user_uuid", m.user_uuid},
        {"title", m.title},
        {"s3_path", m.s3_path},
        {"is_public", m.is_public}
    };
}


void Session::handle_api(){
    json json_resp;
    http::status status = http::status::ok;

    auto target = req.target();
    auto method = req.method();

    if(target == "/api/status" && method == http::verb::get){
        json_resp = {{"status", "online"}, {"secure", true}};

    }else if(method == http::verb::post && target == "/music/my"){
        auto body = json::parse(req.body());
        std::string access_token = body.value("access_token", "");
        auto jwt = verify_jwt(access_token);
        if(jwt.first){
            std::vector<music_file> target_music = server.db.get_All_Users_Music(jwt.second);
            json_resp = {{"music_list: ", target_music}};

        }else{
            std::cout << "expried: "  << std::endl;
            status = http::status::unauthorized;
            json_resp = {{"status", "expried or unaothorized"}};
        }
    }else if(method == http::verb::post && target == "/music/my/upload"){
        auto body = json::parse(req.body());
    }
    else{
        status = http::status::not_found;
        json_resp = {{"status", "not found"}};
    }

    auto resp = std::make_shared<http::response<http::string_body>>(status, req.version());
    resp->set(http::field::server, "asio Igore-Corp Server");
    resp->set(http::field::content_type, "application/json");
    resp->keep_alive(req.keep_alive());
    resp->body() = json_resp.dump();
    resp->prepare_payload();


    auto self = shared_from_this();

    http::async_write(ssl_stream, *resp,
                      [self, resp](beast::error_code er, size_t bytes){
        if(!resp->keep_alive()){
            self->do_close();
        }else if(!er){
            self->do_read();
        }else if(er){
            std::cerr << "error in handle write: " << er.what() << std::endl;
        }
    });

}


void Session::start_streaming(){
    auto resp = std::make_shared<http::response<http::empty_body>>(http::status::ok, req.version());
    resp->set(http::field::server, "Streaming server music.mp3");
    resp->set(http::field::content_type, "audio/mpeg");
    resp->chunked(true);

    auto ser = std::make_shared<http::response_serializer<http::empty_body>>(*resp);

    auto self = shared_from_this();
    http::async_write_header(ssl_stream, *ser,
                             [self, resp, ser](beast::error_code er, size_t bytes){
        if(!er){
            self->open_file_read();
            self->stream_loop();
        }else{
            std::cerr << "error in start streaming: " << er.what() << std::endl;
        }
    });
}

void Session::stream_loop()
{

    int readed = read_file_chunk();

    if(readed <= 0){
        auto final_chunk = http::make_chunk_last();
        auto self = shared_from_this();

        asio::async_write(ssl_stream, final_chunk,
                          [self](boost::system::error_code er, size_t bytes){
            if(!er){
                self->do_read();
            }
        });
        return;
    }



    auto chunk_data = http::make_chunk(beast::net::buffer(buff, readed));
    auto self = shared_from_this();



    asio::async_write(ssl_stream, chunk_data,
                       [self](beast::error_code err, size_t bytes){
        if(!err){
            self->stream_timer->expires_after(std::chrono::milliseconds(50));
            self->stream_timer->async_wait([self](boost::system::error_code err1){
                if(!err1){
                    self->stream_loop();
                }else{
                    std::cerr << "error in async wait: " << err1.what() << std::endl;
                }
            });
        }else{
            std::cerr << "error in stream loop: " << err.what() << std::endl;
        }
    });

}

void Session::open_file_read()
{
    file_stream.open("music.mp3", std::ios::in | std::ios::binary);
    if(!file_stream.is_open()){
        std::cerr << "File Not Open "  << std::endl;
    }
}

std::streamsize Session::read_file_chunk()
{
    if(file_stream.eof()){ return 0; }
    file_stream.read(buff.data(), buff.size());
    return file_stream.gcount();
}

boost::uuids::uuid Session::generate_uuid()
{
    boost::uuids::basic_random_generator<std::mt19937> gen;
    return gen();
}

std::pair<bool, std::string> Session::verify_jwt(const std::string &token)try
{
    auto verify = jwt::verify<jwt::traits::nlohmann_json>()
        .allow_algorithm(jwt::algorithm::hs256{server.secret})
        .with_issuer("auth-manager-server");

    auto decode = jwt::decoded_jwt<jwt::traits::nlohmann_json>(token);
    verify.verify(decode);

    auto claim = decode.get_payload_claim("uuid");
    std::cout << "claim: " << claim.as_string() << std::endl;

    return std::pair<bool, std::string>{true, claim.as_string()};

}catch(std::exception& ex){
    std::cerr << "error in verify_jwt: " << ex.what()  <<"\n";
    return std::pair<bool, std::string>{false, ""};
}

std::pair<bool, minio::s3::BucketExistsResponse> Session::check_bucket(const std::string &bucket)
{
    minio::s3::BucketExistsArgs args;
    args.bucket = bucket;
    minio::s3::BucketExistsResponse response = server.s3Client->BucketExists(args);
    if(response.exist) { return std::pair<bool, minio::s3::BucketExistsResponse> {true, response}; }
    else { return std::pair<bool, minio::s3::BucketExistsResponse> {false, response}; }
}

void Session::do_close()
{
    auto self = shared_from_this();
    ssl_stream.async_shutdown([self](beast::error_code er){
        if(er == beast::net::error::eof){ er = {}; }
        if(er){
            std::cerr << "error in shutdown: " << er.what() << std::endl;
        }
    });
}
