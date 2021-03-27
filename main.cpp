#include <tbb/concurrent_queue.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <pthread.h>
#include "server.h"

int main(int argc, char* argv[]) {
    std::cout << argv[1] << "\t" << argv[2] << std::endl;
    Server server(argv[1], argv[2]);
    server.server_bind();
    
    std::thread main_loop{&Server::run, &server};
    
    main_loop.join();
    return 0;
}
