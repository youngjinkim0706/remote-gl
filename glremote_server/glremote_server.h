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
#include <GL/glew.h>
#include <GLFW/glfw3.h>
// #include <GL/glut.h>


#include "../glremote/gl_commands.h"

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
        
        std::string ip_address;
        std::string port;
        bool enableStreaming;

        GLFWwindow *window;
        static int framebufferWidth, framebufferHeight;

        int fd;
        Server(char* address, char* p, bool flag){
            ip_address = std::string(address);
            port = std::string(p);
            enableStreaming = flag;
            
            if(enableStreaming){
                if (access(FIFO_NAME,F_OK) == 0) {
                    unlink(FIFO_NAME);
                }

                if(mkfifo(FIFO_NAME,0666) == -1){
                    printf("fail to call fifo()\n");
                    exit(1);
                }

                if((fd = open(FIFO_NAME, O_WRONLY)) < 0){
                    printf("fail to open fifo()\n");
                    exit(1);
                }
            }
        };
        ~Server(){
            // sock.close();
            // ctx.close();
            // glfwTerminate();
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