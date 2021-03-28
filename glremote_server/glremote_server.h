#include <zmq.hpp>
#include <string.h>
#include <iostream>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
// #include <GL/glut.h>


#include "../glremote/gl_commands.h"

#define BUFFER_SIZE 1024
#define WIDTH 1024
#define HEIGHT 768

class Server{
    public:
        // static zmq::context_t ctx;
        // static zmq::socket_t sock;
        zmq::context_t ctx;
        zmq::socket_t sock;
        
        zmq::context_t ipc_ctx;
        zmq::socket_t ipc_sock;
        
        std::string ip_address;
        std::string port;
        bool enableStreaming;

        GLFWwindow *window;
        static int framebufferWidth, framebufferHeight;

        Server(char* address, char* p, bool flag){
            ip_address = std::string(address);
            port = std::string(p);
            enableStreaming = flag;
        };
        ~Server(){
            // sock.close();
            // ctx.close();
            // glfwTerminate();
        }
        void server_bind();
        void run();
        // static void run();
              
        void init_gl();

};