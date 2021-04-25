#include <zmq.hpp>
#include <string.h>
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
// #include <GL/glut.h>


#include "gl_commands.h"

#define BUFFER_SIZE 1024
#define WIDTH 1024
#define HEIGHT 768
#define FIFO_NAME "splab_stream"
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
              
        void init_gl();

};