#include "glremote_server.h"
#include <vector>

int Server::framebufferHeight = 0;
int Server::framebufferWidth = 0;
int shader_compiled = 0;
static void errorCallback(int errorCode, const char *errorDescription)

{
    fprintf(stderr, "Error: %s\n", errorDescription);
}

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam ){
    
    if(type == GL_DEBUG_TYPE_ERROR){
        fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
        exit (-1);
    }
}

void Server::server_bind()
{
    sock = zmq::socket_t(ctx, zmq::socket_type::rep);
     
    sock.bind("tcp://" + ip_address + ":" + port);

    if (enableStreaming)
    {
        boost::interprocess::message_queue::remove(streaming_queue_name.c_str());
        std::cout << streaming_queue_name.c_str() << std::endl;
        size_t max_msg_size = WIDTH * HEIGHT * 4;
        mq.reset(new boost::interprocess::message_queue(boost::interprocess::create_only
                                                , streaming_queue_name.c_str()
                                                , 1024
                                                , max_msg_size));

        // ipc_sock = zmq::socket_t(ipc_ctx, zmq::socket_type::push);
        // ipc_sock.bind("ipc:///tmp/streamer.zmq"); // need to change
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
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
         std::exit(EXIT_FAILURE);
    }    
     
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glEnable              ( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( MessageCallback, 0 );
     
     

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
        // std::cout << c->cmd << std::endl;
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
                   
                    char* data_str = new char[size+1];
                    strcpy(data_str, data.c_str());
                    strings.push_back(data_str);

                }else{
                    strings.push_back("\n");
                }
            }

            glShaderSource(cmd_data->shader, cmd_data->count, &strings[0], NULL);
            break;
        }
        case GLSC_glTransformFeedbackVaryings:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTransformFeedbackVaryings_t *cmd_data = (gl_glTransformFeedbackVaryings_t *)data_msg.data();
            std::vector<const char *> strings;
            for (int i = 0; i < cmd_data->count; i++)
            {
                zmq::message_t more_data;
                auto res = sock.recv(more_data, zmq::recv_flags::none);
                if(!more_data.empty()){
                    
                    std::string data(more_data.to_string());
                    size_t size = data.size();
                   
                    char* data_str = new char[size+1];
                    strcpy(data_str, data.c_str());
                    strings.push_back(data_str);

                }else{
                    strings.push_back("\n");
                }
            }

            glTransformFeedbackVaryings(cmd_data->program, cmd_data->count, &strings[0], cmd_data->bufferMode);
            break;
        }
        case GLSC_glCompileShader:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCompileShader_t *cmd_data = (gl_glCompileShader_t *)data_msg.data();
            glCompileShader(cmd_data->shader);

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

            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &result, sizeof(int));
           
            break;
        }
        case GLSC_glReadPixels:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glReadPixels_t *cmd_data = (gl_glReadPixels_t *)data_msg.data();
            int size = 0;
            
            switch (cmd_data->type) {
                case GL_UNSIGNED_BYTE: {
                    if (cmd_data->format == GL_RGBA) {
                        size = cmd_data->width * cmd_data->height * 4;
                    } else if (cmd_data->format == GL_RGB) {
                        size = cmd_data->width * cmd_data->height * 3;
                    }
                    break;
                }
                default:
                    break;
            }

            void* result = malloc(size);
            glReadPixels(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height, cmd_data->format, cmd_data->type, result);
            
            ret.rebuild(size);
            
            memcpy(ret.data(), result, size);
           
            break;
        }
        case GLSC_glBlendFuncSeparate:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBlendFuncSeparate_t *cmd_data = (gl_glBlendFuncSeparate_t *)data_msg.data();
            glBlendFuncSeparate(cmd_data->sfactorRGB, cmd_data->dfactorRGB, cmd_data->sfactorAlpha, cmd_data->dfactorAlpha);
             
            break;
        }
        case GLSC_glDisableVertexAttribArray:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDisableVertexAttribArray_t *cmd_data = (gl_glDisableVertexAttribArray_t *)data_msg.data();
            glDisableVertexAttribArray(cmd_data->index);
             
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
            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &result, sizeof(int));
             
            break;
        }
        case GLSC_glGetError:
        {
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
            GLuint* result = new GLuint[cmd_data->n];
            glGenBuffers(cmd_data->n, result);

            ret.rebuild(sizeof(GLuint) * cmd_data->n);
            memcpy(ret.data(), result, sizeof(GLuint) * cmd_data->n);
             
            break;
        }
        case GLSC_glGenRenderbuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenRenderbuffers_t *cmd_data = (gl_glGenRenderbuffers_t *)data_msg.data();
            GLuint* result = new GLuint[cmd_data->n];
            glGenRenderbuffers(cmd_data->n, result);

            ret.rebuild(sizeof(GLuint) * cmd_data->n);
            memcpy(ret.data(), result, sizeof(GLuint) * cmd_data->n);
             
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
        case GLSC_glDeleteTextures:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDeleteTextures_t *cmd_data = (gl_glDeleteTextures_t *)data_msg.data();
            
            GLuint texture;

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            memcpy(&texture, more_data.data(), sizeof(GLuint) * cmd_data->n);
            glDeleteTextures(cmd_data->n, &texture);
            break;
        }
        case GLSC_glDeleteFramebuffers:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDeleteFramebuffers_t *cmd_data = (gl_glDeleteFramebuffers_t *)data_msg.data();
            
            GLuint framebuffer;
            
            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            memcpy(&framebuffer, more_data.data(), sizeof(GLuint) * cmd_data->n);
            glDeleteFramebuffers(cmd_data->n, &framebuffer);
            break;
        }
        case GLSC_glDeleteRenderbuffers:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDeleteRenderbuffers_t *cmd_data = (gl_glDeleteRenderbuffers_t *)data_msg.data();
            
            GLuint renderbuffer;
            
            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            memcpy(&renderbuffer, more_data.data(), sizeof(GLuint) * cmd_data->n);
            glDeleteRenderbuffers(cmd_data->n, &renderbuffer);
            break;
        }
        case GLSC_glClearBufferfv:{
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glClearBufferfv_t *cmd_data = (gl_glClearBufferfv_t *)data_msg.data();
            
            GLfloat* buffer_data = new GLfloat[4];
            
            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            memcpy(buffer_data, more_data.data(), sizeof(GLfloat) * 4);
            glClearBufferfv(cmd_data->buffer, cmd_data->drawbuffer, buffer_data);
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
        case GLSC_glClearDepthf:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glClearDepthf_t *cmd_data = (gl_glClearDepthf_t *)data_msg.data();
            glClearDepth(cmd_data->d);
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
        case GLSC_glBufferSubData:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glBufferSubData_t *cmd_data = (gl_glBufferSubData_t *)data_msg.data();
            void *buffer_data = malloc(cmd_data->size);
            // float buffer_data[9];

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), cmd_data->size);
                glBufferSubData(cmd_data->target, cmd_data->offset, cmd_data->size, buffer_data);
            }else{
                glBufferSubData(cmd_data->target, cmd_data->offset, cmd_data->size, NULL);
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
            GLfloat *buffer_data = new GLfloat[4 * cmd_data->count];
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniform4fv(cmd_data->location, cmd_data->count, buffer_data);
            }else{
                glUniform4fv(cmd_data->location, cmd_data->count, NULL);
            }

            break;
        }
        case GLSC_glVertexAttrib4fv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glVertexAttrib4fv_t *cmd_data = (gl_glVertexAttrib4fv_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLfloat *buffer_data = new GLfloat[4];
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glVertexAttrib4fv(cmd_data->index, buffer_data);
            }else{
                glVertexAttrib4fv(cmd_data->index, NULL);
            }

            break;
        }
        case GLSC_glDrawElements:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glDrawElements_t *cmd_data = (gl_glDrawElements_t *)data_msg.data();
            
            glDrawElements(cmd_data->mode, cmd_data->count, cmd_data->type, (void *)cmd_data->indices);
            break;
        }
        case GLSC_glUniform2fv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glUniform2fv_t *cmd_data = (gl_glUniform2fv_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLfloat *buffer_data = new GLfloat[2 * cmd_data->count];
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniform2fv(cmd_data->location, cmd_data->count, buffer_data);
            }else{
                glUniform2fv(cmd_data->location, cmd_data->count, NULL);
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
            GLfloat *buffer_data = new GLfloat[16 * cmd_data->count];
            
            if(!more_data.empty()){
                memcpy((void *)buffer_data, more_data.data(), more_data.size());
                glUniformMatrix4fv(cmd_data->location, cmd_data->count, cmd_data->transpose, buffer_data);
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

            GLuint* result = new GLuint[cmd_data->n];
            glGenVertexArrays(cmd_data->n, result);

            ret.rebuild(sizeof(GLuint) * cmd_data->n);
            memcpy(ret.data(), result, sizeof(GLuint) * cmd_data->n);
             
            break;
        }
        case GLSC_glDrawBuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glDrawBuffers_t *cmd_data = (gl_glDrawBuffers_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLenum *buffer_data = new GLenum[cmd_data->n];
            
            memcpy((void *)buffer_data, more_data.data(), more_data.size());
            glDrawBuffers(cmd_data->n, buffer_data);
           
            break;
        }
        case GLSC_glDeleteVertexArrays:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glDeleteVertexArrays_t *cmd_data = (gl_glDeleteVertexArrays_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLuint *buffer_data = new GLuint[cmd_data->n];
            
            memcpy((void *)buffer_data, more_data.data(), more_data.size());
            glDeleteVertexArrays(cmd_data->n, buffer_data);
           
            break;
        }
        case GLSC_glDeleteBuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glDeleteBuffers_t *cmd_data = (gl_glDeleteBuffers_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLuint *buffer_data = new GLuint[cmd_data->n];
            
            memcpy((void *)buffer_data, more_data.data(), more_data.size());
            glDeleteBuffers(cmd_data->n, buffer_data);
           
            break;
        }
        case GLSC_glReadBuffer:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glReadBuffer_t *cmd_data = (gl_glReadBuffer_t *)data_msg.data();
            glReadBuffer(cmd_data->src);
             
            break;
        }
        case GLSC_glBlitFramebuffer:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBlitFramebuffer_t *cmd_data = (gl_glBlitFramebuffer_t *)data_msg.data();
            glBlitFramebuffer(cmd_data->srcX0, cmd_data->srcY0, cmd_data->srcX1, cmd_data->srcY1, 
                                cmd_data->dstX0, cmd_data->dstY0, cmd_data->dstX1, cmd_data->dstY1, cmd_data->mask, cmd_data->filter);
             
            break;
        }
        case GLSC_glBindVertexArray:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindVertexArray_t *cmd_data = (gl_glBindVertexArray_t *)data_msg.data();
        
            glBindVertexArray(cmd_data->array);
        
            break;
        }
        case GLSC_glGetAttribLocation:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetAttribLocation_t *cmd_data = (gl_glGetAttribLocation_t *)data_msg.data();

            char *buffer_data;
            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            int positionAttr = glGetAttribLocation(cmd_data->programObj, more_data.to_string().c_str());

            hasReturn = true;
            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &positionAttr, sizeof(int));
             
            break;
        }
        case GLSC_glBindAttribLocation:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBindAttribLocation_t *cmd_data = (gl_glBindAttribLocation_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            glBindAttribLocation(cmd_data->program, cmd_data->index, more_data.to_string().c_str());
 
            break;
        }
        case GLSC_glGetUniformLocation:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetUniformLocation_t *cmd_data = (gl_glGetUniformLocation_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLint location  = glGetUniformLocation(cmd_data->program, more_data.to_string().c_str());

            hasReturn = true;
            ret.rebuild(sizeof(GLint));
            memcpy(ret.data(), &location, sizeof(GLint));
             
            break;
        }
        case GLSC_glGetUniformBlockIndex:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetUniformBlockIndex_t *cmd_data = (gl_glGetUniformBlockIndex_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);
            GLint index = glGetUniformBlockIndex(cmd_data->program, more_data.to_string().c_str());

            hasReturn = true;
            ret.rebuild(sizeof(GLint));
            memcpy(ret.data(), &index, sizeof(GLint));
             
            break;
        }
        case GLSC_glGetStringi:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetStringi_t *cmd_data = (gl_glGetStringi_t *)data_msg.data();

            const GLubyte *strings = glGetStringi(cmd_data->name, cmd_data->index);
            std::string result;

            result.append(reinterpret_cast<const char *>(strings)); // new style
            hasReturn = true;
            ret.rebuild(result.size());
            memcpy(ret.data(), result.data(), result.size());
            
            break;
        }
        case GLSC_glGetString:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetString_t *cmd_data = (gl_glGetString_t *)data_msg.data();

            const GLubyte *strings = glGetString(cmd_data->name);
            std::string result;

            result.append(reinterpret_cast<const char *>(strings)); // new style
            hasReturn = true;
            ret.rebuild(result.size());
            memcpy(ret.data(), result.data(), result.size());
            // const GLubyte *strings2 = reinterpret_cast<const GLubyte *>(ret);
            
            // std::cout << result2 << std::endl;
            break;
        }
        case GLSC_glVertexAttribPointer:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glVertexAttribPointer_t *cmd_data = (gl_glVertexAttribPointer_t *)data_msg.data();
            glVertexAttribPointer(cmd_data->index, cmd_data->size, cmd_data->type, cmd_data->normalized, cmd_data->stride, (void *) cmd_data->pointer); // pointer add 0
         
            break;
        }
        case GLSC_glEnableVertexAttribArray:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glEnableVertexAttribArray_t *cmd_data = (gl_glEnableVertexAttribArray_t *)data_msg.data();
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
            glClearColor(cmd_data->red, cmd_data->green, cmd_data->blue, cmd_data->alpha);
      
            break;
        }
        case GLSC_glDrawArrays:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDrawArrays_t *cmd_data = (gl_glDrawArrays_t *)data_msg.data();
            glDrawArrays(cmd_data->mode, cmd_data->first, cmd_data->count);
      
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
            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &result, sizeof(GLint));

            break;
        }
        case GLSC_glGetFloatv:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGetFloatv_t *cmd_data = (gl_glGetFloatv_t *)data_msg.data();

            GLfloat result;
            glGetFloatv(cmd_data->pname, &result);
            ret.rebuild(sizeof(int));
            memcpy(ret.data(), &result, sizeof(GLint));
       
            break;
        }
        case GLSC_glGenTextures:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenTextures_t *cmd_data = (gl_glGenTextures_t *)data_msg.data();

            GLuint* result = new GLuint[cmd_data->n];
            glGenTextures(cmd_data->n, result);

            ret.rebuild(sizeof(GLuint) * cmd_data->n);
            memcpy(ret.data(), result, sizeof(GLuint) * cmd_data->n);

            break;
        }
        case GLSC_glGenFramebuffers:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glGenFramebuffers_t *cmd_data = (gl_glGenFramebuffers_t *)data_msg.data();

            GLuint* result = new GLuint[cmd_data->n];
            glGenFramebuffers(cmd_data->n, result);

            ret.rebuild(sizeof(GLuint) * cmd_data->n);
            memcpy(ret.data(), result, sizeof(GLuint) * cmd_data->n);

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
            glFrontFace(cmd_data->mode);

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
        case GLSC_glFramebufferTextureLayer:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glFramebufferTextureLayer_t *cmd_data = (gl_glFramebufferTextureLayer_t *)data_msg.data();
            glFramebufferTextureLayer(cmd_data->target, cmd_data->attachment, cmd_data->texture, cmd_data->level, cmd_data->layer);

            break;
        }
        case GLSC_glRenderbufferStorageMultisample:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glRenderbufferStorageMultisample_t *cmd_data = (gl_glRenderbufferStorageMultisample_t *)data_msg.data();
            glRenderbufferStorageMultisample(cmd_data->target, cmd_data->samples, cmd_data->internalformat, cmd_data->width, cmd_data->height);
            
            break;
        }
        case GLSC_glUniform1f:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glUniform1f_t *cmd_data = (gl_glUniform1f_t *)data_msg.data();
            glUniform1f(cmd_data->location, cmd_data->v0);
            
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
        case GLSC_glCompressedTexImage2D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCompressedTexImage2D_t *cmd_data = (gl_glCompressedTexImage2D_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);

            if(!more_data.empty()){                 
                const void *buffer_data = malloc(more_data.size());
                memcpy((void *)buffer_data, more_data.data(), more_data.size());                
                glCompressedTexImage2D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height,
                         cmd_data->border, cmd_data->imageSize, buffer_data);            
            }
            else{
                glCompressedTexImage2D(cmd_data->target, cmd_data->level, cmd_data->internalformat,
                         cmd_data->width, cmd_data->height,
                         cmd_data->border, cmd_data->imageSize, NULL);     
            }             
            break;
        }
        case GLSC_glTexSubImage2D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTexSubImage2D_t *cmd_data = (gl_glTexSubImage2D_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);

            if(!more_data.empty()){                 
                const void *buffer_data = malloc(more_data.size());
                memcpy((void *)buffer_data, more_data.data(), more_data.size());                
                glTexSubImage2D(cmd_data->target, cmd_data->level, cmd_data->xoffset, cmd_data->yoffset,
                         cmd_data->width, cmd_data->height,
                         cmd_data->format,
                         cmd_data->type, buffer_data);            
            }
            else{
                glTexSubImage2D(cmd_data->target, cmd_data->level, cmd_data->xoffset, cmd_data->yoffset,
                         cmd_data->width, cmd_data->height,
                         cmd_data->format,
                         cmd_data->type, NULL);     
            }             
            break;
        }
        case GLSC_glTexImage3D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTexImage3D_t *cmd_data = (gl_glTexImage3D_t *)data_msg.data();

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
        case GLSC_glTexSubImage3D:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);

            gl_glTexSubImage3D_t *cmd_data = (gl_glTexSubImage3D_t *)data_msg.data();

            zmq::message_t more_data;
            res = sock.recv(more_data, zmq::recv_flags::none);

            if(!more_data.empty()){                 
                const void *buffer_data = malloc(more_data.size());
                memcpy((void *)buffer_data, more_data.data(), more_data.size());                
                glTexSubImage3D(cmd_data->target, cmd_data->level, cmd_data->xoffset, cmd_data->yoffset, cmd_data->zoffset,
                         cmd_data->width, cmd_data->height, cmd_data->depth,
                         cmd_data->format,
                         cmd_data->type, buffer_data);            
            }
            else{
                glTexSubImage3D(cmd_data->target, cmd_data->level, cmd_data->xoffset, cmd_data->yoffset, cmd_data->zoffset,
                         cmd_data->width, cmd_data->height, cmd_data->depth,
                         cmd_data->format,
                         cmd_data->type, NULL);        
            }             
            break;
        }
        case GLSC_glUniform1ui:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glUniform1ui_t *cmd_data = (gl_glUniform1ui_t *) data_msg.data();
            glUniform1ui(cmd_data->location, cmd_data->v0);
            break;    
        }
        case GLSC_glBeginTransformFeedback:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glBeginTransformFeedback_t *cmd_data = (gl_glBeginTransformFeedback_t *) data_msg.data();
            glBeginTransformFeedback(cmd_data->primitiveMode);
            break;    
        }
        case GLSC_glEndTransformFeedback:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glEndTransformFeedback_t *cmd_data = (gl_glEndTransformFeedback_t *) data_msg.data();
            glEndTransformFeedback();
            break;    
        }
        case GLSC_glVertexAttribDivisor:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glVertexAttribDivisor_t *cmd_data = (gl_glVertexAttribDivisor_t *) data_msg.data();
            glVertexAttribDivisor(cmd_data->index, cmd_data->divisor);
            break;    
        }
        case GLSC_glDrawArraysInstanced:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glDrawArraysInstanced_t *cmd_data = (gl_glDrawArraysInstanced_t *) data_msg.data();
            glDrawArraysInstanced(cmd_data->mode, cmd_data->first, cmd_data->count, cmd_data->instancecount);
            break;    
        }
        case GLSC_glCullFace:
        {
            zmq::message_t data_msg;
            auto res = sock.recv(data_msg, zmq::recv_flags::none);
            gl_glCullFace_t *cmd_data = (gl_glCullFace_t *) data_msg.data();
            glCullFace(cmd_data->mode);
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
                try{
                    mq.get()->send(pixel_data, WIDTH * HEIGHT * 4, 0);
                    // std::cout << mq.get()->get_num_msg() << std::endl;
                }
                catch(boost::interprocess::interprocess_exception &ex){
                      std::cout << ex.what() << std::endl;
                }
            }
            break;
        }
        default:
            break;
        }
        sock.send(ret, zmq::send_flags::none);
        usleep(0.001);
    }
}