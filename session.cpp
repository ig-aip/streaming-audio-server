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
                            if(self->req.method() == http::verb::get && self->req.target() == "/music.mp3"){
                                self->start_streaming();
                            }else{
                                self->handle_api();
                            }
                         }
                         else if(er == http::error::end_of_stream || er == asio::ssl::error::stream_truncated){ self->do_close(); }
                         else{
                            std::cerr << "error in asyc read: " << er.what() << std::endl;
                         }
                     });
}

void Session::handle_api(){
    json json_resp;
    http::status status = http::status::ok;

    if(req.target() == "/api/status"){
        json_resp = {{"status", "online"}, {"secure", true}};
    }else{
        status = http::status::not_found;
        json_resp = {"status", "not found"};
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
