#include "database.h"

DataBase::DataBase() :
    conn_str(getConnStrFromFile("connection.txt")),
    conn_pool(getConnStrFromFile("connection.txt"), 5)
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
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params("SELECT id, user_uuid, title, s3_path, is_public FROM music_files WHERE user_uuid = $1 AND is_public = TRUE", user_uuid);

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
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params("SELECT id, user_uuid, title, s3_path, is_public FROM music_files WHERE user_uuid = $1", user_uuid);

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
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params(
        "INSERT INTO music_files(user_uuid, title, s3_path, is_public) VALUES ($1, $2, $3, $4) RETURNING id",
                                user_uuid, title, s3_path, is_public);

    work.commit();

    if(result.empty()){
        std::cout << "Error Put music, response. empty"  << std::endl;
        return "";
    }
    return result[0]["id"].as<std::string>();
}catch(std::exception& ex){
    std::cerr << "Error in put music: " << ex.what() << "\n";
    return "";
}



Conn_guard::Conn_guard(ConnectionPool &pool_) : pool(pool_), conn(pool.get())
{
}

Conn_guard::~Conn_guard(){
    pool.release(conn);
}

std::shared_ptr<pqxx::connection> Conn_guard::get()
{
    return conn;
}
