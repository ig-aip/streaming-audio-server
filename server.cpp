#include "Server.h"
#include "algorithm"


Server::Server()  :
    ioc(),
    acceptor(ioc),
    ctx(ssl::context::tls_server),
    secret(load_secret())
{
    boost::system::error_code er;
    acceptor.open(ip::tcp::endpoint{ip::make_address_v4(IP), PORT}.protocol(), er);

    if(er){
        throw std::runtime_error("error in open acceptor:" + er.what());
    }

    acceptor.set_option(asio::socket_base::reuse_address(true), er);
    if(er){
        throw std::runtime_error("error in open set option:" + er.what());
    }

    acceptor.bind(ip::tcp::endpoint{ip::make_address_v4(IP), PORT}, er);
    if(er){
        throw std::runtime_error("error in bind:" + er.what());
    }

    acceptor.listen(asio::socket_base::max_listen_connections, er);
    if(er){
        throw std::runtime_error("error in listen:" + er.what());
    }


    s3CLientInit();

}


void Server::s3CLientInit() try
{
    minio::s3::BaseUrl baseUrl;

    baseUrl.host = "127.0.0.1";
    baseUrl.port = 9000;
    baseUrl.https = false;

    s3Provider = std::make_unique<minio::creds::StaticProvider>("minioadmin", "minioadmin");
    s3Client = std::make_unique<minio::s3::Client>(baseUrl, s3Provider.get());
}catch(std::exception& ex){
    std::cerr <<"Error in s3Client Initialization: " << ex.what()  << "\n";
}

std::pair<bool, minio::s3::BucketExistsResponse> Server::check_bucket(const std::string &bucket)
{
    minio::s3::BucketExistsArgs args;
    args.bucket = bucket;
    minio::s3::BucketExistsResponse response = s3Client->BucketExists(args);
    if(response.exist) { return std::pair<bool, minio::s3::BucketExistsResponse> {true, response}; }
    else { return std::pair<bool, minio::s3::BucketExistsResponse> {false, response}; }
}

void Server::create_bucket_ifnot_exists(const std::string& bucket)try
{
    if(check_bucket(bucket).first){
        return;
    }
    minio::s3::MakeBucketArgs args;
    args.bucket = bucket;

    minio::s3::MakeBucketResponse resp = s3Client->MakeBucket(args);
    if(!resp){
        std::cout << "bucket: " << resp.Error() << " cancelled" <<std::endl;;
        return;
    }else{
        std::cout << "bucket: " << bucket << " created" <<std::endl;;
    }
    return;
}catch(std::exception& ex){
    std::cerr << "error in create bucket: " << ex.what()  <<"\n";
}

void Server::start_acceptor()try
{
    std::cout << "starting" << std::endl;
    auto sock = std::make_shared<ip::tcp::socket>(ioc);
    auto self = shared_from_this();

    acceptor.async_accept(*sock, [self, sock](boost::system::error_code er){
        if(!er){

            auto session = std::make_shared<Session>(*self, sock, self->ctx);
            session->run();
            self->start_acceptor();
        }else{
            throw std::runtime_error("error in accept: " + er.message());
        }
    });
}catch(std::exception& ex){
    std::cerr << "error in start acceptor: " << ex.what() << '\n';
}

std::string Server::load_secret()try
{
    std::ifstream secretFile("secret.txt", std::ios_base::binary);
    if(!secretFile){
        std::cerr << "secret not open \n";
        return "";
    }

    std::string secretStr;
    std::getline(secretFile, secretStr);
    if(secretFile){ secretFile.close(); }

    return secretStr;
}catch(std::exception& ex){
    std::cerr << "error in load secret: " <<ex.what() <<"\n";
    return "";
}



void Server::start()try
{

    load_server_certificate(ctx);
    create_bucket_ifnot_exists(music_bucket);
    start_acceptor();

    unsigned int threads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    pool.reserve(threads);
    auto self = shared_from_this();
    for(int i = 0 ; i < threads; ++i){
        pool.emplace_back([self]() -> void{
            try {
                self->ioc.run();
            }
            catch (std::exception ex) {
                std::cerr << "error int start poll threads: " << std::endl;
            }
        });
    }


    for(auto& t : pool){
        t.join();
    }
}catch(std::exception& ex){
    std::cerr <<"error in start: " << ex.what() << '\n';
}


void Server::load_server_certificate(asio::ssl::context& contx){
    try {
            contx.set_options(asio::ssl::context::default_workarounds |
                asio::ssl::context::no_sslv2 |
                asio::ssl::context::single_dh_use);
            contx.use_certificate_chain_file("server.crt");
            contx.use_private_key_file("server.key", asio::ssl::context::pem);
    }
    catch (std::exception& ex) {
        std::cout << "exception in load certificate" << std::endl;
    }

}
