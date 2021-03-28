#include "glremote_server.h"

int Server::framebufferHeight = 0;
int Server::framebufferWidth = 0;

static void errorCallback(int errorCode, const char* errorDescription) 

{
	fprintf(stderr, "Error: %s\n", errorDescription);
}

void Server::server_bind(){
    sock = zmq::socket_t(ctx, zmq::socket_type::rep);
    std::cout << "tcp://" + ip_address + ":" + port << std::endl;
    sock.bind("tcp://" + ip_address + ":" + port);

    if(enableStreaming){
        ipc_sock = zmq::socket_t(ipc_ctx, zmq::socket_type::push);
        ipc_sock.bind("ipc:///tmp/streamer.zmq"); // need to change
    } 
}


static void framebufferSizeCallback(GLFWwindow* window, int width, int height){
	//처음 2개의 파라미터는 viewport rectangle의 왼쪽 아래 좌표
	//다음 2개의 파라미터는 viewport의 너비와 높이이다.
	//framebuffer의 width와 height를 가져와 glViewport에서 사용한다.
	glViewport(0, 0, width, height);

	Server::framebufferWidth = width;
	Server::framebufferHeight = height;
}

void Server::init_gl(){

    glfwSetErrorCallback(errorCallback);   
	if (!glfwInit()) { 	 
		std::cerr << "Error: GLFW 초기화 실패" << std::endl;
		std::exit(EXIT_FAILURE);
	}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow( 
		WIDTH, 
		HEIGHT, 
		"Remote Drawing Example", 
		NULL, NULL);

	if (!window) {
		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    glewExperimental = GL_TRUE;
	GLenum errorCode = glewInit();  
	if (GLEW_OK != errorCode) {

		std::cerr << "Error: GLEW 초기화 실패 - " << std::endl;

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl; 
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    
    // glfwSwapInterval(1);
}

void Server::run(){
    bool quit = false;
    init_gl();
    
	double lastTime = glfwGetTime(); 
	int numOfFrames = 0;
	int count = 0;
    while(!quit){
        //waiting until data comes here
        zmq::message_t msg;
        zmq::message_t ret;
        bool hasReturn = false;

        auto res = sock.recv(msg, zmq::recv_flags::none);
        gl_command_t* c = (gl_command_t*) msg.data();
        // std::cout << c->cmd << std::endl;
        switch (c->cmd)
        {
            case GLSC_glClear:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glClear_t* cmd_data = (gl_glClear_t*) data_msg.data();
                glClear(cmd_data->mask);
                break;
            }
            case GLSC_glBegin:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glBegin_t* cmd_data = (gl_glBegin_t*) data_msg.data();
                glBegin(cmd_data->mode);
                break;
            }
            case GLSC_glColor3f:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glColor3f_t* cmd_data = (gl_glColor3f_t*) data_msg.data();

                glColor3f(cmd_data->red, cmd_data->green, cmd_data->blue);
                break;
            }
            case GLSC_glVertex3f:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glVertex3f_t* cmd_data = (gl_glVertex3f_t*) data_msg.data();

                glVertex3f(cmd_data->x, cmd_data->y, cmd_data->z);
                break;
            }
            case GLSC_glEnd:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glEnd_t* cmd_data = (gl_glEnd_t*) data_msg.data();
                glEnd();
                break;
            }
            case GLSC_glFlush:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glFlush_t* cmd_data = (gl_glFlush_t*) data_msg.data();
                glFlush();
                break;
            }
            case GLSC_glCreateShader:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glCreateShader_t* cmd_data = (gl_glCreateShader_t*) data_msg.data();
                unsigned int shader = glCreateShader(cmd_data->type);
                std::cout << cmd_data->type << std::endl;
                hasReturn = true;
                ret.rebuild(sizeof(unsigned int));
                memcpy(ret.data(), &shader, sizeof(unsigned int));
                break;
            }
            case GLSC_glShaderSource:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                
                gl_glShaderSource_t* cmd_data = (gl_glShaderSource_t*) data_msg.data();
                char* string_data[cmd_data->count];

                for(int i=0; i < cmd_data->count; i++){
                    zmq::message_t more_data;
                    auto res = sock.recv(more_data, zmq::recv_flags::none);
                    string_data[i] = (char*) malloc(strlen((char *)more_data.data()));
                    memcpy((void *)string_data[i], more_data.data(), strlen((char *)more_data.data()));
                }
                std::cout << cmd_data->shader << "\t" << cmd_data->count << "\n" << string_data[cmd_data->count - 1] << std::endl;
                
            
                glShaderSource(cmd_data->shader, cmd_data->count, string_data, NULL);
                break;
            }
            case GLSC_glCompileShader:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glCompileShader_t* cmd_data = (gl_glCompileShader_t*) data_msg.data();
                std::cout << cmd_data->shader <<std::endl;
                glCompileShader(cmd_data->shader);
                break;
            }
            case GLSC_glGetShaderiv:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glGetShaderiv_t* cmd_data = (gl_glGetShaderiv_t*) data_msg.data();
                int result;
                glGetShaderiv(cmd_data->shader, cmd_data->pname, &result);
	            if (!result){
                    GLchar errorLog[512];
                    glGetShaderInfoLog(cmd_data->shader, 512, NULL, errorLog);
                    std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
                }

                msg.rebuild(sizeof(int));
                memcpy(msg.data(), &result, sizeof(int));
                break;
            }
            case GLSC_glCreateProgram:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glCreateProgram_t* cmd_data = (gl_glCreateProgram_t*) data_msg.data();
                unsigned int program = glCreateProgram();
                std::cout << program << std::endl;
                hasReturn = true;
                ret.rebuild(sizeof(unsigned int));
                memcpy(ret.data(), &program, sizeof(unsigned int));
                break;
            }
            case GLSC_glAttachShader:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glAttachShader_t* cmd_data = (gl_glAttachShader_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glAttachShader(cmd_data->program, cmd_data->shader);
                break;
            }
            case GLSC_glLinkProgram:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glLinkProgram_t* cmd_data = (gl_glLinkProgram_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glLinkProgram(cmd_data->program);
                break;
            }
            case GLSC_glGetProgramiv:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glGetProgramiv_t* cmd_data = (gl_glGetProgramiv_t*) data_msg.data();
                int result;
                glGetProgramiv(cmd_data->program, cmd_data->pname, &result);
	            if (!result){
                    GLchar errorLog[512];
                    glGetShaderInfoLog(cmd_data->program, 512, NULL, errorLog);
                    std::cerr <<  "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
                }
                msg.rebuild(sizeof(int));
                memcpy(msg.data(), &result, sizeof(int));
                std::cout << "program" << result << std::endl;
                break;
            }
            case GLSC_glGenBuffers:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glGenBuffers_t* cmd_data = (gl_glGenBuffers_t*) data_msg.data();
                unsigned int result;
                glGenBuffers(cmd_data->n, &result);
                
                msg.rebuild(sizeof(int));
                memcpy(msg.data(), &result, sizeof(unsigned int));
                std::cout << "glGenBuffers" << result << std::endl;
                break; 
            }
            case GLSC_glBindBuffer:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glBindBuffer_t* cmd_data = (gl_glBindBuffer_t*) data_msg.data();
                glBindBuffer(cmd_data->target, cmd_data->id);
                break;
            }
            case GLSC_glBufferData:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                
                gl_glBufferData_t* cmd_data = (gl_glBufferData_t*) data_msg.data();
                void* buffer_data = malloc(cmd_data->size);
                // float buffer_data[9];

                zmq::message_t more_data;
                res = sock.recv(more_data, zmq::recv_flags::none);
                memcpy((void *)buffer_data, more_data.data(), cmd_data->size);
                
                glBufferData(cmd_data->target, cmd_data->size, buffer_data, cmd_data->usage);
                break;
            }
            case GLSC_glGenVertexArrays:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glGenVertexArrays_t* cmd_data = (gl_glGenVertexArrays_t*) data_msg.data();
                
                unsigned int result;
                glGenVertexArrays(cmd_data->n, &result);

                msg.rebuild(sizeof(int));
                memcpy(msg.data(), &result, sizeof(unsigned int));
                std::cout << "glGenVertexArrays" << result << std::endl;
                break;
            }

            case GLSC_glBindVertexArray:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glBindVertexArray_t* cmd_data = (gl_glBindVertexArray_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glBindVertexArray(cmd_data->array);
                break;
            }
            case GLSC_glGetAttribLocation:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glGetAttribLocation_t* cmd_data = (gl_glGetAttribLocation_t*) data_msg.data();
                
                char* buffer_data;
                // float buffer_data[9];

                zmq::message_t more_data;
                res = sock.recv(more_data, zmq::recv_flags::none);
                int positionAttr = glGetAttribLocation(cmd_data->programObj, more_data.to_string().c_str());
               
                hasReturn = true;
                ret.rebuild(sizeof(int));
                memcpy(ret.data(), &positionAttr, sizeof(int));
                break;
            }
            case GLSC_glVertexAttribPointer:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glVertexAttribPointer_t* cmd_data = (gl_glVertexAttribPointer_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                // glBindVertexArray(cmd_data->array);
                glVertexAttribPointer(cmd_data->index, cmd_data->size, cmd_data->type, cmd_data->normalized, cmd_data->stride, 0); // pointer add 0
                break;
            }
            case GLSC_glEnableVertexAttribArray:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glEnableVertexAttribArray_t* cmd_data = (gl_glEnableVertexAttribArray_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                // glBindVertexArray(cmd_data->array);
                glEnableVertexAttribArray(cmd_data->index); 
                break;
            }
            case GLSC_glUseProgram:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glUseProgram_t* cmd_data = (gl_glUseProgram_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glUseProgram(cmd_data->program);
                break;
            }
            case GLSC_glClearColor:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glClearColor_t* cmd_data = (gl_glClearColor_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glClearColor(cmd_data->red, cmd_data->green, cmd_data->blue, cmd_data->alpha);
                break;
            }
            case GLSC_glDrawArrays:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glDrawArrays_t* cmd_data = (gl_glDrawArrays_t*) data_msg.data();
                // std::cout << cmd_data->shader <<std::endl;
                glDrawArrays(cmd_data->mode, cmd_data->first, cmd_data->count);
                break;
            }
            case GLSC_bufferSwap:{
                double currentTime = glfwGetTime();
                numOfFrames++;
                if (currentTime - lastTime >= 1.0) { 

                    printf("%f ms/frame  %d fps \n", 1000.0 / double(numOfFrames), numOfFrames);
                    numOfFrames = 0;
                    lastTime = currentTime;
                }

                glfwSwapBuffers(window);
                glfwPollEvents();
                
                // streaming
                if (enableStreaming){
                    unsigned char *pixel_data  = (unsigned char*) malloc(WIDTH * HEIGHT * 4); // GL_RGBA
                    glReadBuffer(GL_FRONT);
                    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
                    
                    zmq::message_t zmq_pixel_data(WIDTH * HEIGHT * 4);     
                    memcpy(zmq_pixel_data.data(), (void *)pixel_data, WIDTH * HEIGHT * 4);
                    ipc_sock.send(zmq_pixel_data, zmq::send_flags::none);
                }
                break;
            }
            case GLSC_glViewport:{
                zmq::message_t data_msg;
                auto res = sock.recv(data_msg, zmq::recv_flags::none);
                gl_glViewport_t* cmd_data = (gl_glViewport_t*) data_msg.data();
                std::cout << cmd_data->width << "\t" << cmd_data->height << std::endl;
                glViewport(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height);
                break;
            }
            case GLSC_BREAK:{
                quit = true;
                break;
            }
            default:
                break;
        }
        if (!hasReturn){
            sock.send(msg, zmq::send_flags::none);
        }else{
            sock.send(ret, zmq::send_flags::none);
        }
    }
}