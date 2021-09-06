#include <chrono>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
// #include <unordered_map>
#include "lru_cache.hpp"
#include <zmq.hpp>
#include <zmq_addon.hpp>
// #include <GL/glew.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <bitset>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>
#include <snappy.h>
// #include <GL/glut.h>

#include "gl_commands.h"

#define BUFFER_SIZE 1024
#define WIDTH 3840
#define HEIGHT 2160
#define FIFO_NAME "splab_stream"
#define TCP_MINIMUM_PACKET_SIZE 20
#define CACHE_KEY_SIZE sizeof(size_t) * __CHAR_BIT__ + sizeof(unsigned char) * __CHAR_BIT__
typedef std::string cache_key;
typedef struct
{
    gl_command_t cmd;
    cache_key data_key;
    cache_key more_data_key;
} record_t;

class Server
{
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
    Server(char *address, char *p, bool flag, const char *q_name = "message_queue")
    {
        ip_address = std::string(address);
        port = std::string(p);
        enableStreaming = flag;
        if (enableStreaming)
        {
            streaming_queue_name = std::string(q_name);
        }
    };
    ~Server()
    {
        // sock.close();
        // ctx.close();
        glfwTerminate();
        if (enableStreaming)
        {
            unlink(FIFO_NAME);
            close(fd);
        }
    }
    void server_bind();
    void run();
    // static void run();
    std::string insert_or_check_cache(lru11::Cache<cache_key, std::string> &cache, unsigned char cmd, zmq::message_t &data_msg);
    std::string recv_data(zmq::socket_t &socket, unsigned char cmd, bool is_cached, lru11::Cache<cache_key, std::string> &cache, bool is_recored);
    std::string alloc_cached_data(zmq::message_t &data_msg);
    cache_key create_cache_key(unsigned char cmd, std::size_t hashed_data);
    void append_record(gl_command_t *c, std::string data, std::string more_data);
    void append_record(gl_command_t *c, std::string data);

    void init_gl();
};