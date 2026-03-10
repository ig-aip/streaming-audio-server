#include "database.h"

DataBase::DataBase() :
    conn_str(getConnStrFromFile("connection.txt")),
    conn_pool(conn_str, 5)
{

}

std::string DataBase::getConnStrFromFile(const std::string fileName)try
{
    std::ifstream conn_file(fileName, std::ios_base::binary);
    if(!conn_file.is_open()){
        std::cerr << "Connection file not open \n";
        return "";
    }
    std::string str;
    std::getline(conn_file, str);
    return str;

}catch(std::exception& ex){
    std::cerr << "Error in open connection file: " << ex.what() << '\n';
    return "";
}

std::vector<music_file> DataBase::get_Public_Users_Music(const std::string& user_uuid)try
{
    std::vector<music_file> targetList;
    pqxx::work work(*conn_pool.get());
    pqxx::result result = work.exec_params("SELECT id user_uuid, title, s3_path, is_public FROM music_files WHERE user_uuid = $1 AND is_public = TRUE", user_uuid);

    for(const auto& row : result){
        music_file target;
        target.id = row["id"].as<std::string>();
        target.title = row["title"].as<std::string>();
        target.user_uuid = row["user_uuid"].as<std::string>();
        target.s3_path = row["s3_path"].as<std::string>();
        target.is_public = row["is_public"].as<bool>();

        targetList.push_back(target);
    }

    return targetList;
}catch(std::exception& ex){
    std::cerr << "Error in get public music list: " << ex.what() << "\n";
    return std::vector<music_file>{};
}


std::vector<music_file> DataBase::get_All_Users_Music(const std::string& user_uuid)try
{
    std::vector<music_file> targetList;
    pqxx::work work(*conn_pool.get());
    pqxx::result result = work.exec_params("SELECT id user_uuid, title, s3_path, is_public FROM music_files WHERE user_uuid = $1", user_uuid);

    for(const auto& row : result){
        music_file target;
        target.id = row["id"].as<std::string>();
        target.title = row["title"].as<std::string>();
        target.user_uuid = row["user_uuid"].as<std::string>();
        target.s3_path = row["s3_path"].as<std::string>();
        target.is_public = row["is_public"].as<bool>();

        targetList.push_back(target);
    }

    return targetList;
}catch(std::exception& ex){
    std::cerr << "Error in get public music list: " << ex.what() << "\n";
    return std::vector<music_file>{};
}

std::string DataBase::put_music(const std::string &user_uuid, const std::string &title, const std::string &s3_path, bool is_public)try
{
    pqxx::work work(*conn_pool.get());
    pqxx::result result = work.exec_params(
        "INSER INTO music_files(user_uuid, title, s3_path, is_public) VALUES ($1, $2, $3, $4) RETURNING id",
                                user_uuid, title, s3_path, is_public);

    work.commit();

    return result[0]["id"].as<std::string>();
}catch(std::exception& ex){
    std::cerr << "Error in put music: " << ex.what() << "\n";
    return "";
}



Scoped_conn_pool::Scoped_conn_pool(const std::string &conn_str, size_t size):
    conn_pool(conn_str, size),
    conn(nullptr)
{

}

std::shared_ptr<pqxx::connection> Scoped_conn_pool::get()
{
    if(conn != nullptr){
        conn_pool.release(conn);
    }

    conn = conn_pool.get();

    return conn;
}

void Scoped_conn_pool::release(std::shared_ptr<pqxx::connection> old_con)
{
    if(conn != nullptr){
        conn_pool.release(conn);
        conn = nullptr;
    }
}

Scoped_conn_pool::~Scoped_conn_pool()
{
    release(conn);
}
