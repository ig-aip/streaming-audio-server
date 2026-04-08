#include "connectionpool.h"


std::shared_ptr<pqxx::connection> ConnectionPool::get()try
{
    std::unique_lock<std::mutex> lock(mutex);
    while(pool.empty()){
        cond.wait(lock);
    }

    auto conn = pool.front();
    pool.pop();
    if(!conn->is_open()){
        conn = std::make_shared<pqxx::connection>(conn_str);
    }

    return conn;
}catch(std::exception& ex){
    std::cerr << "Error in get sql connection: " << ex.what() <<'\n';
}

ConnectionPool::ConnectionPool(const std::string &conn_str_, const size_t size_)try : conn_str {conn_str_}, pool_size{size_}
{
    for(size_t i = 0; i < pool_size; ++i){
        auto conn = std::make_shared<pqxx::connection>(conn_str);
        if(conn->is_open()){
            pool.push(conn);
        }
    }
}catch(std::exception& ex){
    std::cerr << "Error in start connection pool: " << ex.what() << " conn_str: " << conn_str_ <<" \n";
}


void ConnectionPool::release(std::shared_ptr<pqxx::connection> oldConn)try
{
    if(oldConn){
        if(oldConn->is_open()){
            pqxx::work work(*oldConn);
            work.exec("ROLLBACK");
            work.commit();
        }else{
            oldConn = std::make_shared<pqxx::connection>(conn_str);
        }
    }

    std::unique_lock<std::mutex> lock (mutex);
    pool.push(oldConn);
    lock.unlock();
    cond.notify_one();
}catch(std::exception& ex){
    std::cerr << "Error in release sql connection: " << ex.what() <<'\n';
}






