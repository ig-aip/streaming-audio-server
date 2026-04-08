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

    pqxx::result result = work.exec_params(
        "SELECT m.id, m.user_uuid, m.title, m.s3_path, m.is_public, u.username, "
        "EXISTS( SELECT 1 FROM user_likes ul WHERE ul.music_id = m.id AND ul.user_uuid = $1) AS is_liked "
        "FROM music_files m JOIN users u ON m.user_uuid = u.uuid WHERE m.is_public = TRUE ORDER BY m.listens_count DESC",
        user_uuid);

    for(const auto& row : result){
        music_file target;
        target.id = row["id"].as<std::string>();
        target.title = row["title"].as<std::string>();
        target.user_uuid = row["user_uuid"].as<std::string>();
        target.s3_path = row["s3_path"].as<std::string>();
        target.username = row["username"].as<std::string>();
        target.is_public = row["is_public"].as<bool>();
        target.is_liked = row["is_liked"].as<bool>();

        targetList.push_back(target);
    }

    std::cout << "get_Public_Users_Music size: " << targetList.size() << std::endl;

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
    pqxx::result result = work.exec_params(
        "SELECT m.id, m.user_uuid, m.title, m.s3_path, m.is_public, u.username,"
        "EXISTS(SELECT 1 FROM user_likes ul WHERE ul.music_id = m.id AND ul.user_uuid = $1) as is_liked "
        "FROM music_files m JOIN users u ON m.user_uuid = u.uuid WHERE m.user_uuid = $1",
        user_uuid);

    for(const auto& row : result){
        music_file target;
        target.id = row["id"].as<std::string>();
        target.title = row["title"].as<std::string>();
        target.user_uuid = row["user_uuid"].as<std::string>();
        target.s3_path = row["s3_path"].as<std::string>();
        target.username = row["username"].as<std::string>();
        target.is_public = row["is_public"].as<bool>();
        target.is_liked = row["is_liked"].as<bool>();

        targetList.push_back(target);
    }

    std::cout << "target list size: " << targetList.size() << std::endl;
    return targetList;
}catch(std::exception& ex){
    std::cerr << "Error in get public music list: " << ex.what() << "\n";
    return std::vector<music_file>{};
}

std::vector<music_file> DataBase::get_liked_music(const std::string &user_uuid)
{
    std::vector<music_file> target_music;
    try {
        Conn_guard guard(conn_pool);
        pqxx::work work(*guard.get());

        pqxx::result result = work.exec_params(
            "SELECT m.id, m.user_uuid, m.title, m.s3_path, m.is_public, u.username "
            "FROM music_files m "
            "JOIN user_likes ul ON m.id = ul.music_id "
            "JOIN users u ON m.user_uuid = u.uuid "
            "WHERE ul.user_uuid = $1 "
            "ORDER BY ul.created_at DESC",
            user_uuid
            );

        for(const auto& row : result){
            music_file m;
            m.id = row["id"].as<std::string>();
            m.is_public = row["is_public"].as<bool>();
            m.s3_path = row["s3_path"].as<std::string>();
            m.username = row["username"].as<std::string>();
            m.title = row["title"].as<std::string>();
            m.user_uuid = row["user_uuid"].as<std::string>();
            m.is_liked = true;

            target_music.push_back(m);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error in get_liked_music: " << ex.what() << '\n';
    }
    return target_music;
}

bool DataBase::like_music(const std::string &user_uuid, const std::string &music_id)try
{
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params(
        "INSERT INTO user_likes(user_uuid, music_id) VALUES($1, $2) ON CONFLICT DO NOTHING",
        user_uuid, music_id);

    work.commit();
    return true;
}catch(std::exception& ex){
    std::cerr << "error in like_music: " << ex.what() << '\n';
    return false;
}

bool DataBase::unlike_music(const std::string &user_uuid, const std::string &music_id)try
{
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params(
        "DELETE FROM user_likes WHERE user_uuid = $1 AND music_id = $2",
        user_uuid, music_id);

    work.commit();
    return true;
}catch(std::exception& ex){
    std::cerr << "error in unlike_music: " << ex.what() << '\n';
    return false;
}

bool DataBase::increment_listens(const std::string &music_id)try
{
    Conn_guard guard(conn_pool);
    pqxx::work work(*guard.get());
    pqxx::result result = work.exec_params(
        "UPDATE music_files SET listens_count = listens_count + 1 WHERE id = $1", music_id);

    work.commit();
    return true;
}catch(std::exception& ex){
    std::cerr << "error in increment_listens: " << ex.what() << '\n';
    return false;
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

std::vector<music_file> DataBase::search_music(const std::string& query, const std::string& user_uuid) {
    std::vector<music_file> targetList;
    try {
        Conn_guard guard(conn_pool);
        pqxx::work work(*guard.get());
        pqxx::result result = work.exec_params(
            "SELECT m.id, m.user_uuid, m.title, m.s3_path, m.is_public, u.username, "
            "EXISTS(SELECT 1 FROM user_likes ul WHERE ul.music_id = m.id AND ul.user_uuid = $1) as is_liked "
            "FROM music_files m "
            "JOIN users u ON m.user_uuid = u.uuid " // ДОБАВЛЕН JOIN
            "WHERE m.is_public = TRUE AND m.title ILIKE $2 "
            "ORDER BY m.listens_count DESC",
            user_uuid, "%" + query + "%");

        for(const auto& row : result){
            music_file target;
            target.id = row["id"].as<std::string>();
            target.title = row["title"].as<std::string>();
            target.user_uuid = row["user_uuid"].as<std::string>();
            target.s3_path = row["s3_path"].as<std::string>();
            target.username = row["username"].as<std::string>();
            target.is_public = row["is_public"].as<bool>();
            target.is_liked = row["is_liked"].as<bool>();

            targetList.push_back(target);
        }
    } catch(std::exception& ex){
        std::cerr << "Error in search_music: " << ex.what() << "\n";
    }
    return targetList;
}

std::pair<int, int> DataBase::get_user_stats(const std::string& user_uuid) {
    try {
        Conn_guard guard(conn_pool);
        pqxx::work work(*guard.get());

        pqxx::result res_uploads = work.exec_params(
            "SELECT COUNT(*) FROM music_files WHERE user_uuid = $1", user_uuid);
        int uploads = res_uploads[0][0].as<int>();

        pqxx::result res_likes = work.exec_params(
            "SELECT COUNT(*) FROM user_likes WHERE user_uuid = $1", user_uuid);
        int likes = res_likes[0][0].as<int>();

        return {uploads, likes};
    } catch(std::exception& ex) {
        std::cerr << "Error in get_user_stats: " << ex.what() << "\n";
        return {0, 0};
    }
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
