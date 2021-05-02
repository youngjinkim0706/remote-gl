#include <chrono>
#include <zmq.hpp>
#include <string.h>
#include <map>
#include <iostream>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstddef>
#include <cstring>
// #include <GL/glew.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <bitset>
// #include <GL/glut.h>


#include "gl_commands.h"

#define BUFFER_SIZE 1024
#define WIDTH 1024
#define HEIGHT 768
#define FIFO_NAME "splab_stream"
typedef unsigned long cache_key;

class Server{
    public:
        // static zmq::context_t ctx;
        // static zmq::socket_t sock;
        zmq::context_t ctx;
        zmq::socket_t sock;
        
        zmq::context_t ipc_ctx;
        zmq::socket_t ipc_sock;

        boost::scoped_ptr<boost::interprocess::message_queue> mq;
        std::string ip_address;
        std::string port;
        std::string streaming_queue_name;
        bool enableStreaming;
        int sequence_number;

        std::map<cache_key, std::string> data_cache;
        std::map<cache_key, std::string> more_data_cache;


        std::map<unsigned int, unsigned int> glGenBuffers_idx_map;
        std::map<unsigned int, unsigned int> glGenVertexArrays_idx_map;
        std::map<unsigned int, unsigned int> glGenTextures_idx_map;
        std::map<unsigned int, unsigned int> glGenFramebuffers_idx_map;
        std::map<unsigned int, unsigned int> glGenRenderbuffers_idx_map;
        std::map<unsigned int, unsigned int> glGenQueries_idx_map;
        std::map<unsigned int, unsigned int> glGenSamplers_idx_map;
        std::map<unsigned int, unsigned int> glGenTransformFeedbacks_idx_map;

        GLFWwindow *window;
        static int framebufferWidth, framebufferHeight;

        int fd;
        Server(char* address, char* p, bool flag, const char* q_name="message_queue"){
            ip_address = std::string(address);
            port = std::string(p);
            enableStreaming = flag;
            if(enableStreaming){
                streaming_queue_name = std::string(q_name);
            }
        };
        ~Server(){
            // sock.close();
            // ctx.close();
            glfwTerminate();
            if(enableStreaming){
                unlink(FIFO_NAME);
                close(fd);
            }

        }
        void server_bind();
        void run();
        // static void run();
        std::string insert_or_check_cache(std::map<cache_key, std::string> cache, cache_key key, zmq::message_t &data_msg);
        std::string alloc_cached_data(zmq::message_t &data_msg);
        bool check_more_cache(std::map<std::string, std::string> cache, std::string key, std::string value, std::string data);
        cache_key cache_key_gen(unsigned char cmd, int sequence_number);
        std::string get_value_from_request(zmq::message_t &msg);
        void init_gl();

};