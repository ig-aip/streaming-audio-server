#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include <pqxx/pqxx>
#include <memory>
#include <queue>
#include <condition_variable>
#include <iostream>

class ConnectionPool
{
    std::string conn_str;
    std::queue<std::shared_ptr<pqxx::connection>> pool;
    std::condition_variable cond;
    std::mutex mutex;
    size_t pool_size;
public:
    std::shared_ptr<pqxx::connection> get();
    void release(std::shared_ptr<pqxx::connection> oldConn);
    ConnectionPool(const std::string& conn_str_, const size_t size_);
};

#endif // CONNECTIONPOOL_H
