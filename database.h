#ifndef DATABASE_H
#define DATABASE_H
#include "net.h"
#include "connectionpool.h"
#include "fstream"



struct music_file{
    std::string id;
    std::string user_uuid;
    std::string s3_path;
    std::string title;
    bool is_public;

};

class Scoped_conn_pool{
    ConnectionPool conn_pool;
    std::shared_ptr<pqxx::connection> conn;

public:
    Scoped_conn_pool(const std::string& conn_str, size_t size);
    std::shared_ptr<pqxx::connection> get();
    void release(std::shared_ptr<pqxx::connection> old_con);
    ~Scoped_conn_pool();
};


class Conn_guard{
    ConnectionPool& pool;
    std::shared_ptr<pqxx::connection> conn;
public:
    Conn_guard(ConnectionPool& pool_);
    ~Conn_guard();

    std::shared_ptr<pqxx::connection> get();

};



class DataBase
{
    ConnectionPool conn_pool;
    std::string conn_str;
    std::string getConnStrFromFile(const std::string fileName);

public:
    std::vector<music_file> get_Public_Users_Music(const std::string& user_uuid);
    std::vector<music_file> get_All_Users_Music(const std::string& user_uuid);

    std::string put_music(const std::string& user_uuid, const std::string& title, const std::string& s3_path, bool is_public);
    DataBase();
};



#endif // DATABASE_H
