#ifndef NET_H
#define NET_H
#include "boost/beast.hpp"
#include "boost/asio.hpp"
#include "boost/uuid.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/beast/ssl/ssl_stream.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include "settings.h"

using json = nlohmann::json;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace ip = asio::ip;
namespace http = beast::http;
namespace ssl = asio::ssl;
using uuid = boost::uuids::uuid;
using tcp = boost::asio::ip::tcp;




#endif // NET_H
