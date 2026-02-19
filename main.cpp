#include "server.h"

int main(int argc, char *argv[])
{
    auto server = std::make_shared<Server>();
    server->start();
}
