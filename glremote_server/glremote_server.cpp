#include "glremote_server.h"
#include <vector>

int Server::framebufferHeight = 0;
int Server::framebufferWidth = 0;
int shader_compiled = 0;
static void errorCallback(int errorCode, const char *errorDescription)

{
    fprintf(stderr, "Error: %s\n", errorDescription);
}

void Server::server_bind()
{
    sock = zmq::socket_t(ctx, zmq::socket_type::rep);
     
    sock.bind("tcp://" + ip_address + ":" + port);

    if (enableStreaming)
    {
        ipc_sock = zmq::socket_t(ipc_ctx, zmq::socket_type::push);
        ipc_sock.bind("ipc:///tmp/streamer.zmq"); // need to change
    }
}

static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    //처음 2개의 파라미터는 viewport rectangle의 왼쪽 아래 좌표
    //다음 2개의 파라미터는 viewport의 너비와 높이이다.
    //framebuffer의 width와 height를 가져와 glViewport에서 사용한다.
    glViewport(0, 0, width, height);

    Server::framebufferWidth = width;
    Server::framebufferHeight = height;
}

void Server::init_gl()
{

    glfwSetErrorCallback(errorCallback);
    if (!glfwInit())
    {
        std::cerr << "Error: GLFW 초기화 실패" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

    window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Remote Drawing Example",
        NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glewExperimental = GL_TRUE;
    GLenum errorCode = glewInit();
    if (GLEW_OK != errorCode)
    {

        std::cerr << "Error: GLEW 초기화 실패 - " << std::endl;

        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }
     
     
     
     

    // glfwSwapInterval(1);
}

void Server::run()
{
    bool quit = false;
    init_gl();

    double lastTime = glfwGetTime();
    int numOfFrames = 0;
    int count = 0;
    while (!quit)
    {
        //waiting until data comes here
        zmq::message_t msg;
        zmq::message_t ret;
        bool hasReturn = false;

        auto res = sock.recv(msg, zmq::recv_flags::none);
        gl_command_t *c = (gl_command_t *)msg.data();
        //  
        switch (c->cmd)
        {
        case GLSC_glClear:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glClear_t *cmd_data = (gl_glClear_t *)data_msg.data();
            glClear(cmd_data->mask);
             

            break;
        }
        case GLSC_glBegin:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBegin_t *cmd_data = (gl_glBegin_t *)data_msg.data();
            glBegin(cmd_data->mode);
             

            break;
        }
        case GLSC_glColor3f:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glColor3f_t *cmd_data = (gl_glColor3f_t *)data_msg.data();

            glColor3f(cmd_data->red, cmd_data->green, cmd_data->blue);
             

            break;
        }
        case GLSC_glVertex3f:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glVertex3f_t *cmd_data = (gl_glVertex3f_t *)data_msg.data();

            glVertex3f(cmd_data->x, cmd_data->y, cmd_data->z);
             

            break;
        }
        case GLSC_glEnd:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glEnd_t *cmd_data = (gl_glEnd_t *)data_msg.data();
            glEnd();
             

            break;
        }
        case GLSC_glFlush:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glFlush_t *cmd_data = (gl_glFlush_t *)data_msg.data();
            glFlush();
            
             

            break;
        }
        case GLSC_glCreateShader:
        {
            
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCreateShader_t *cmd_data = (gl_glCreateShader_t *)data_msg.data();
            unsigned int shader = glCreateShader(cmd_data->type);
            
            hasReturn = true;
            ret.rebuild(sizeof(unsigned int));
            memcpy(ret.data(), &shader, sizeof(unsigned int));
             

            break;
        }
        case GLSC_glShaderSource:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glShaderSource_t *cmd_data = (gl_glShaderSource_t *)data_msg.data();

            std::vector<const char *> strings;
            for (int i = 0; i < cmd_data->count; i++)
            {
                zmq::message_t more_data;
                auto res = sock.recv(more_data, zmq::recv_flags::none);
                if(!more_data.empty()){
                    
                    std::string data(more_data.to_string());
                    size_t size = data.size();

                    char* data_str = new char[size];
                    strcpy(data_str, data.c_str());
                    strings.push_back(data_str);

                }else{
                    strings.push_back("\n");
                }

            }

            glShaderSource(cmd_data->shader, cmd_data->count, &strings[0], NULL);
            
            break;
        }
        case GLSC_glCompileShader:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCompileShader_t *cmd_data = (gl_glCompileShader_t *)data_msg.data();
            glCompileShader(cmd_data->shader);
            // std::cout << "GLSC_glCompileShader end" << std::endl;

            break;
        }
        case GLSC_glGetShaderiv:
        {
             
    
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetShaderiv_t *cmd_data = (gl_glGetShaderiv_t *)data_msg.data();
            int result;
            glGetShaderiv(cmd_data->shader, cmd_data->pname, &result);
            shader_compiled++;
            if (!result)
            {
                GLchar errorLog[512];
                glGetShaderInfoLog(cmd_data->shader, 512, NULL, errorLog);
                std::cerr << "ERROR: vertex shader 컴파일 실패\n"
                          << errorLog << std::endl;
            }

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(int));
             

            break;
        }
        case GLSC_glCreateProgram:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCreateProgram_t *cmd_data = (gl_glCreateProgram_t *)data_msg.data();
            unsigned int program = glCreateProgram();
             
            hasReturn = true;
            ret.rebuild(sizeof(unsigned int));
            memcpy(ret.data(), &program, sizeof(unsigned int));

            break;
        }
        case GLSC_glCheckFramebufferStatus:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCheckFramebufferStatus_t *cmd_data = (gl_glCheckFramebufferStatus_t *)data_msg.data();
            unsigned int status = glCheckFramebufferStatus(cmd_data->target);
             
            hasReturn = true;
            ret.rebuild(sizeof(unsigned int));
            memcpy(ret.data(), &status, sizeof(unsigned int));

            break;
        }
        case GLSC_glAttachShader:
        {

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glAttachShader_t *cmd_data = (gl_glAttachShader_t *)data_msg.data();
            glAttachShader(cmd_data->program, cmd_data->shader);
             
            break;
        }

        case GLSC_glDisable:
        {

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDisable_t *cmd_data = (gl_glDisable_t *)data_msg.data();
            glDisable(cmd_data->cap);
             
            break;
        }
        case GLSC_glEnable:
        {

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glEnable_t *cmd_data = (gl_glEnable_t *)data_msg.data();
            glEnable(cmd_data->cap);
             
            break;
        }
        case GLSC_glLinkProgram:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glLinkProgram_t *cmd_data = (gl_glLinkProgram_t *)data_msg.data();
            //  
            glLinkProgram(cmd_data->program);
            break;
        }
        case GLSC_glGetProgramiv:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetProgramiv_t *cmd_data = (gl_glGetProgramiv_t *)data_msg.data();
            int result;
            glGetProgramiv(cmd_data->program, cmd_data->pname, &result);
            if (!result)
            {
                GLchar errorLog[512];
                glGetShaderInfoLog(cmd_data->program, 512, NULL, errorLog);
                std::cerr << "ERROR: shader program 연결 실패\n"
                          << errorLog << std::endl;
            }
            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(int));
             
            break;
        }
        case GLSC_glGetError:{
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetError_t *cmd_data = (gl_glGetError_t *)data_msg.data();
            unsigned int error = glGetError();
             
            
            hasReturn = true;
            ret.rebuild(sizeof(unsigned int));
            memcpy(ret.data(), &error, sizeof(unsigned int));
             

            break;
        }
        case GLSC_glGenBuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenBuffers_t *cmd_data = (gl_glGenBuffers_t *)data_msg.data();
            unsigned int result;
            glGenBuffers(cmd_data->n, &result);

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(unsigned int));
             
            break;
        }
        case GLSC_glGenRenderbuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenRenderbuffers_t *cmd_data = (gl_glGenRenderbuffers_t *)data_msg.data();
            unsigned int result;
            glGenRenderbuffers(cmd_data->n, &result);

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(unsigned int));
             
            break;
        }
        case GLSC_glBindRenderbuffer:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindRenderbuffer_t *cmd_data = (gl_glBindRenderbuffer_t *)data_msg.data();
            glBindRenderbuffer(cmd_data->target, cmd_data->renderbuffer);
            
            break;
        }
        case GLSC_glRenderbufferStorage:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glRenderbufferStorage_t *cmd_data = (gl_glRenderbufferStorage_t *)data_msg.data();
            glRenderbufferStorage(cmd_data->target, cmd_data->internalformat, cmd_data->width, cmd_data->height);
            
            break;
        }
        case GLSC_glFramebufferRenderbuffer:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glFramebufferRenderbuffer_t *cmd_data = (gl_glFramebufferRenderbuffer_t *)data_msg.data();
            glFramebufferRenderbuffer(cmd_data->target, cmd_data->attachment, cmd_data->renderbuffertarget, cmd_data->renderbuffer);
            
            break;
        }
        case GLSC_glBindBuffer:
        {
             
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindBuffer_t *cmd_data = (gl_glBindBuffer_t *)data_msg.data();
            glBindBuffer(cmd_data->target, cmd_data->id);
            
            break;
        }
        case GLSC_glBindFramebuffer:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindFramebuffer_t *cmd_data = (gl_glBindFramebuffer_t *)data_msg.data();
            glBindFramebuffer(cmd_data->target, cmd_data->framebuffer);
            
            break;
        }
        case GLSC_glBindBufferBase:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindBufferBase_t *cmd_data = (gl_glBindBufferBase_t *)data_msg.data();
            glBindBufferBase(cmd_data->target, cmd_data->index, cmd_data-> buffer);
            break;
        }
        case GLSC_glDepthFunc:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDepthFunc_t *cmd_data = (gl_glDepthFunc_t *)data_msg.data();
            glDepthFunc(cmd_data->func);
            break;
        }
        case GLSC_glColorMask:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glColorMask_t *cmd_data = (gl_glColorMask_t *)data_msg.data();
            glColorMask(cmd_data->red, cmd_data->green, cmd_data-> blue, cmd_data->alpha);
            break;
        }
        case GLSC_glFramebufferTexture2D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glFramebufferTexture2D_t *cmd_data = (gl_glFramebufferTexture2D_t *)data_msg.data();
            glFramebufferTexture2D(cmd_data->target, cmd_data->attachment, cmd_data->textarget, cmd_data->texture, cmd_data->level);
            
            break;
        }
        case GLSC_glBufferData:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glBufferData_t *cmd_data = (gl_glBufferData_t *)data_msg.data();
            void *buffer_data = malloc(cmd_data->size);
            // float buffer_data[9];

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), cmd_data->size);
                glBufferData(cmd_data->target, cmd_data->size, buffer_data, cmd_data->usage);
            }else{
                glBufferData(cmd_data->target, cmd_data->size, NULL, cmd_data->usage);
            }

            break;
        }
        case GLSC_glUniform4fv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glUniform4fv_t *cmd_data = (gl_glUniform4fv_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            void *buffer_data = malloc(more_data.size());
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniform4fv(cmd_data->location, cmd_data->count, static_cast<const GLfloat*>(buffer_data));
            }else{
                glUniform4fv(cmd_data->location, cmd_data->count, NULL);
            }

            break;
        }
        case GLSC_glUniform2fv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glUniform2fv_t *cmd_data = (gl_glUniform2fv_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            void *buffer_data = malloc(more_data.size());
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniform4fv(cmd_data->location, cmd_data->count, static_cast<const GLfloat*>(buffer_data));
            }else{
                glUniform4fv(cmd_data->location, cmd_data->count, NULL);
            }

            break;
        }
        case GLSC_glUniformMatrix4fv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glUniformMatrix4fv_t *cmd_data = (gl_glUniformMatrix4fv_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            void *buffer_data = malloc(more_data.size());
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniformMatrix4fv(cmd_data->location, cmd_data->count, cmd_data->transpose, static_cast<const GLfloat*>(buffer_data));
            }else{
                glUniformMatrix4fv(cmd_data->location, cmd_data->count, cmd_data->transpose,  NULL);
            }

            break;
        }
        case GLSC_glGenVertexArrays:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenVertexArrays_t *cmd_data = (gl_glGenVertexArrays_t *)data_msg.data();

            unsigned int result;
            glGenVertexArrays(cmd_data->n, &result);

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(unsigned int));
             
            break;
        }

        case GLSC_glBindVertexArray:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindVertexArray_t *cmd_data = (gl_glBindVertexArray_t *)data_msg.data();
            //  
            glBindVertexArray(cmd_data->array);
             

            break;
        }
        case GLSC_glGetAttribLocation:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetAttribLocation_t *cmd_data = (gl_glGetAttribLocation_t *)data_msg.data();

            char *buffer_data;
            // float buffer_data[9];

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            int positionAttr = glGetAttribLocation(cmd_data->programObj, more_data.to_string().c_str());

            hasReturn = true;
            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &positionAttr, sizeof(int));
             

            break;
        }
        case GLSC_glVertexAttribPointer:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glVertexAttribPointer_t *cmd_data = (gl_glVertexAttribPointer_t *)data_msg.data();
            //  
            // glBindVertexArray(cmd_data->array);
            glVertexAttribPointer(cmd_data->index, cmd_data->size, cmd_data->type, cmd_data->normalized, cmd_data->stride, 0); // pointer add 0
             

            break;
        }
        case GLSC_glEnableVertexAttribArray:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glEnableVertexAttribArray_t *cmd_data = (gl_glEnableVertexAttribArray_t *)data_msg.data();
            //  
            // glBindVertexArray(cmd_data->array);
            glEnableVertexAttribArray(cmd_data->index);
             

            break;
        }
        case GLSC_glUseProgram:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glUseProgram_t *cmd_data = (gl_glUseProgram_t *)data_msg.data();

            glUseProgram(cmd_data->program);
             

            break;
        }
        case GLSC_glClearColor:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glClearColor_t *cmd_data = (gl_glClearColor_t *)data_msg.data();
            //  
            glClearColor(cmd_data->red, cmd_data->green, cmd_data->blue, cmd_data->alpha);
             

            break;
        }
        case GLSC_glDrawArrays:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDrawArrays_t *cmd_data = (gl_glDrawArrays_t *)data_msg.data();
            //  
            glDrawArrays(cmd_data->mode, cmd_data->first, cmd_data->count);
             

            break;
        }
        case GLSC_bufferSwap:
        {
            double currentTime = glfwGetTime();
            numOfFrames++;
            if (currentTime - lastTime >= 1.0)
            {

                printf("%f ms/frame  %d fps \n", 1000.0 / double(numOfFrames), numOfFrames);
                numOfFrames = 0;
                lastTime = currentTime;
            }

            glfwSwapBuffers(window);
            glfwPollEvents();

            // streaming
            if (enableStreaming)
            {
                unsigned char *pixel_data = (unsigned char *)malloc(WIDTH * HEIGHT * 4); // GL_RGBA
                glReadBuffer(GL_FRONT);
                glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);

                // zmq::message_t zmq_pixel_data(WIDTH * HEIGHT * 4);
                // memcpy(zmq_pixel_data.data(), (void *)pixel_data, WIDTH * HEIGHT * 4);
                // ipc_sock.send(zmq_pixel_data, zmq::send_flags::none);

                if (write(fd, pixel_data, WIDTH * HEIGHT * 4) < 0)
                {
                     
                }
            }
            break;
        }
        case GLSC_glViewport:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glViewport_t *cmd_data = (gl_glViewport_t *)data_msg.data();
             
            glViewport(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height);
             

            break;
        }
        case GLSC_glScissor:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glScissor_t *cmd_data = (gl_glScissor_t *)data_msg.data();
            glScissor(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height);
             

        }
        case GLSC_BREAK:
        {
            quit = true;
            break;
        }
        case GLSC_glGetIntegerv:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetIntegerv_t *cmd_data = (gl_glGetIntegerv_t *)data_msg.data();

            GLint result;
            glGetIntegerv(cmd_data->pname, &result);
            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(GLint));
             

            break;
        }
        case GLSC_glGetFloatv:
        {
             

            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetFloatv_t *cmd_data = (gl_glGetFloatv_t *)data_msg.data();

            GLfloat result;
            glGetFloatv(cmd_data->pname, &result);
            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(GLint));
             

            break;
        }
        case GLSC_glGenTextures:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenTextures_t *cmd_data = (gl_glGenTextures_t *)data_msg.data();

            GLuint result;
            glGenTextures(cmd_data->n, &result);

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(GLuint));

            break;
        }
        case GLSC_glGenFramebuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenFramebuffers_t *cmd_data = (gl_glGenFramebuffers_t *)data_msg.data();

            GLuint result;
            glGenFramebuffers(cmd_data->n, &result);

            msg.rebuild(sizeof(int));
            memcpy(msg.data(), &result, sizeof(GLuint));

            break;
        }
        case GLSC_glActiveTexture:
        {
             
    
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glActiveTexture_t *cmd_data = (gl_glActiveTexture_t *)data_msg.data();
            glActiveTexture(cmd_data->texture);
             

            break;
        }
        case GLSC_glBindTexture:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindTexture_t *cmd_data = (gl_glBindTexture_t *)data_msg.data();

            // GLuint result;
            glBindTexture(cmd_data->target, cmd_data->texture);
            break;
        }
        case GLSC_glGenerateMipmap:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenerateMipmap_t *cmd_data = (gl_glGenerateMipmap_t *)data_msg.data();
            glGenerateMipmap(cmd_data->target);

            break;
        }
        case GLSC_glFrontFace:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glFrontFace_t *cmd_data = (gl_glFrontFace_t *)data_msg.data();
            glGenerateMipmap(cmd_data->mode);

            break;
        }
        case GLSC_glDepthMask:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDepthMask_t *cmd_data = (gl_glDepthMask_t *)data_msg.data();
            glDepthMask(cmd_data->flag);

            break;
        }
        case GLSC_glBlendEquation:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBlendEquation_t *cmd_data = (gl_glBlendEquation_t *)data_msg.data();
            glBlendEquation(cmd_data->mode);

            break;
        }
        case GLSC_glBlendFunc:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBlendFunc_t *cmd_data = (gl_glBlendFunc_t *)data_msg.data();
            glBlendFunc(cmd_data->sfactor, cmd_data->dfactor);

            break;
        }
        case GLSC_glVertexAttrib4f:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glVertexAttrib4f_t *cmd_data = (gl_glVertexAttrib4f_t *)data_msg.data();
            glVertexAttrib4f(cmd_data->index, cmd_data->x,cmd_data->y,cmd_data->z,cmd_data->w);
            break;
        }
        case GLSC_glUniform1i:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glUniform1i_t *cmd_data = (gl_glUniform1i_t *)data_msg.data();
            glUniform1i(cmd_data->location, cmd_data->v0);
            break;
        }
        case GLSC_glUniformBlockBinding:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glUniformBlockBinding_t *cmd_data = (gl_glUniformBlockBinding_t *)data_msg.data();
            glUniformBlockBinding(cmd_data->program, cmd_data->uniformBlockIndex, cmd_data->uniformBlockBinding);
            break;
        }
        case GLSC_glPixelStorei:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glPixelStorei_t *cmd_data = (gl_glPixelStorei_t *)data_msg.data();
            glPixelStorei(cmd_data->pname, cmd_data->param);
            break;
        }
        case GLSC_glTexStorage2D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glTexStorage2D_t *cmd_data = (gl_glTexStorage2D_t *)data_msg.data();
            glTexStorage2D(cmd_data->target, cmd_data->levels, cmd_data->internalformat, cmd_data->width, cmd_data->height);

            break;
        }
        case GLSC_glTexParameteri:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glTexParameteri_t *cmd_data = (gl_glTexParameteri_t *)data_msg.data();
            glTexParameteri(cmd_data->target, cmd_data->pname, cmd_data->param);

            break;
        }
        case GLSC_glTexParameterf:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glTexParameterf_t *cmd_data = (gl_glTexParameterf_t *)data_msg.data();
            glTexParameterf(cmd_data->target, cmd_data->pname, cmd_data->param);

            break;
        }
        case GLSC_glTexImage2D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTexImage2D_t *cmd_data = (gl_glTexImage2D_t *)data_msg.data();

            // float buffer_data[9];

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);

            if(!more_data.empty()){                 
                const void *buffer_data = malloc(more_data.size());
                memcpy((void *)buffer_data, more_data.data(), more_data.size());                
                glTexImage2D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height,
                         cmd_data->border, cmd_data->format,
                         cmd_data->type, buffer_data);            
            }
            else{
                glTexImage2D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height,
                         cmd_data->border, cmd_data->format,
                         cmd_data->type, NULL);     
            }             
            break;
        }
        case GLSC_glTexImage3D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTexImage3D_t *cmd_data = (gl_glTexImage3D_t *)data_msg.data();

            // float buffer_data[9];

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);

            if(!more_data.empty()){                 
                const void *buffer_data = malloc(more_data.size());
                memcpy((void *)buffer_data, more_data.data(), more_data.size());                
                glTexImage3D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height, cmd_data->depth,
                         cmd_data->border, cmd_data->format,
                         cmd_data->type, buffer_data);            
            }
            else{
                glTexImage3D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height, cmd_data->depth,
                         cmd_data->border, cmd_data->format,
                         cmd_data->type, NULL);     
            }             
            break;
        }
        default:
            break;
        }
        if (!hasReturn)
        {
            sock.send(msg, zmq::send_flags::none);
        }
        else
        {
            sock.send(ret, zmq::send_flags::none);
        }
    }
}