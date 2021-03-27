// #include <tbb/concurrent_queue.h>
#include <zmq.hpp>
#include <string>
#include <iostream>
// #include <GL/glew.h>
#include "gl.h"
#include "gl_commands.h"

#define GL_SET_COMMAND(PTR, FUNCNAME) gl_##FUNCNAME##_t *PTR = (gl_##FUNCNAME##_t *)malloc(sizeof(gl_##FUNCNAME##_t)); PTR->cmd = GLSC_##FUNCNAME

zmq::context_t ctx2;
zmq::socket_t sock2(ctx2, zmq::socket_type::req);

const char* vertexShaderSource = 
    "#version 330 core\n"
    "in vec3 positionAttribute;"
    "in vec3 colorAttribute;"
    "out vec3 passColorAttribute;"
    "void main()"
    "{"
    "gl_Position = vec4(positionAttribute, 1.0);"
    "passColorAttribute = colorAttribute;"
    "}";

const char* fragmentShaderSource =
    "#version 330 core\n"
    "in vec3 passColorAttribute;"
    "out vec4 fragmentColor;"
    "void main()"
    "{"
    "fragmentColor = vec4(passColorAttribute, 1.0);"
    "}";

float position[] = {
		0.0f,  0.5f, 0.0f, //vertex 1  위 중앙
		0.5f, -0.5f, 0.0f, //vertex 2  오른쪽 아래
		-0.5f, -0.5f, 0.0f //vertex 3  왼쪽 아래
	};

float color[] = {
    1.0f, 0.0f, 0.0f, //vertex 1 : RED (1,0,0)
    0.0f, 1.0f, 0.0f, //vertex 2 : GREEN (0,1,0) 
    0.0f, 0.0f, 1.0f  //vertex 3 : BLUE (0,0,1)
};

void* send_data(unsigned int cmd, void * cmd_data, int size){
    gl_command_t c = {
        cmd, size
    };
    zmq::message_t msg(sizeof(c));

    switch (cmd)
    {
        case GLSC_glShaderSource:{
            memcpy(msg.data(), (void *) &c, sizeof(c));
            gl_command_t* cc = (gl_command_t *)msg.data();

            sock2.send(msg, zmq::send_flags::sndmore);
            
            zmq::message_t data_msg(size);
            memcpy(data_msg.data(), cmd_data, size);
            sock2.send(data_msg, zmq::send_flags::sndmore);
            
            gl_glShaderSource_t* more_data = (gl_glShaderSource_t*) cmd_data;
            for(int i=0; i < more_data->count; i++){
                zmq::message_t string_data(strlen(more_data->string[i]));
                memcpy(string_data.data(),more_data->string[i], strlen(more_data->string[i]));
                if(i == more_data->count -1){
                    sock2.send(string_data, zmq::send_flags::none);
                }else{
                    sock2.send(string_data, zmq::send_flags::sndmore);
                }
            }

            auto res = sock2.recv(msg, zmq::recv_flags::none);
            break;
        }
        case GLSC_glBufferData:{
            memcpy(msg.data(), (void *) &c, sizeof(c));
            gl_command_t* cc = (gl_command_t *)msg.data();
            sock2.send(msg, zmq::send_flags::sndmore);
            
            zmq::message_t data_msg(size);
            memcpy(data_msg.data(), cmd_data, size);
            sock2.send(data_msg, zmq::send_flags::sndmore);
            
            gl_glBufferData_t* more_data = (gl_glBufferData_t*) cmd_data;
            zmq::message_t buffer_data(more_data->size);
            memcpy(buffer_data.data(), more_data->data, more_data->size);
            
            sock2.send(buffer_data, zmq::send_flags::none);
            auto res = sock2.recv(msg, zmq::recv_flags::none);
            break;
        }
        case GLSC_glGetAttribLocation:{
            memcpy(msg.data(), (void *) &c, sizeof(c));
            gl_command_t* cc = (gl_command_t *)msg.data();
            sock2.send(msg, zmq::send_flags::sndmore);
            
            zmq::message_t data_msg(size);
            memcpy(data_msg.data(), cmd_data, size);
            sock2.send(data_msg, zmq::send_flags::sndmore);
            
            gl_glGetAttribLocation_t* more_data = (gl_glGetAttribLocation_t*) cmd_data;
            zmq::message_t buffer_data(strlen(more_data->name));
            memcpy(buffer_data.data(), more_data->name, strlen(more_data->name));
            // std::cout << buffer_data.str() << std::endl;
            sock2.send(buffer_data, zmq::send_flags::none);
            auto res = sock2.recv(msg, zmq::recv_flags::none);
            
            break;
        }
        case GLSC_glDrawArrays:
        case GLSC_glClearColor:
        case GLSC_glClear:
        case GLSC_glUseProgram:
        case GLSC_glEnableVertexAttribArray:
        case GLSC_glVertexAttribPointer:
        case GLSC_glBindVertexArray:
        case GLSC_glGenVertexArrays:
        case GLSC_glBindBuffer:
        case GLSC_glGenBuffers:
        case GLSC_glLinkProgram:
        case GLSC_glAttachShader:
        case GLSC_glCreateProgram:
        case GLSC_glGetShaderiv:
        case GLSC_glGetProgramiv:
        case GLSC_glCompileShader:
        case GLSC_glCreateShader:{
            memcpy(msg.data(), (void *) &c, sizeof(c));
            gl_command_t* cc = (gl_command_t *)msg.data();

            sock2.send(msg, zmq::send_flags::sndmore);
            
            zmq::message_t data_msg(size);
            memcpy(data_msg.data(), cmd_data, size);
            sock2.send(data_msg, zmq::send_flags::none);
            
            auto res = sock2.recv(msg, zmq::recv_flags::none);
            break;
        }
        default:{
            zmq::message_t msg(sizeof(c));
            memcpy(msg.data(), (void *) &c, sizeof(c));
            gl_command_t* cc = (gl_command_t *)msg.data();            
            sock2.send(msg, zmq::send_flags::none);
            auto res = sock2.recv(msg, zmq::recv_flags::none);
            break;
        }
    }
    return msg.data();

}

void glClear(unsigned int mask){
    GL_SET_COMMAND(c, glClear);
    c->mask = mask;
    c->cmd = GLSC_glClear;
    send_data(GLSC_glClear, (void*)c, sizeof(gl_glClear_t));
}

void glBegin(unsigned int mode){
    GL_SET_COMMAND(c, glBegin);
    c->mode = mode;
    c->cmd = GLSC_glBegin;
    send_data(GLSC_glBegin, (void*)c, sizeof(gl_glBegin_t));

}
void glColor3f(float red, float green, float blue){
    GL_SET_COMMAND(c, glColor3f);
    c->red = red;
    c->green = green;
    c->blue = blue;
    c->cmd = GLSC_glColor3f;
    // std::cout << c->red << c->green << c->blue << std::endl;

    send_data(GLSC_glColor3f, (void*)c, sizeof(gl_glColor3f_t));
    free(c);
}
void glVertex3f(float x, float y, float z){
    GL_SET_COMMAND(c, glVertex3f);
    c->x = x;
    c->y = y;
    c->z = z;
    c->cmd = GLSC_glVertex3f;

    send_data(GLSC_glVertex3f, (void*)c, sizeof(gl_glVertex3f_t));
    free(c);

}

void glEnd(){
    GL_SET_COMMAND(c, glEnd);
    c->cmd = GLSC_glEnd;
    send_data(GLSC_glEnd, (void*)c, sizeof(gl_glEnd_t));
}

void glFlush(){
    GL_SET_COMMAND(c, glFlush);
    c->cmd = GLSC_glFlush;
    send_data(GLSC_glFlush, (void*)c, sizeof(gl_glFlush_t));
}

void glBreak(){
    gl_command_t* c = (gl_command_t*) malloc(sizeof(gl_command_t));
    c->cmd = GLSC_BREAK;
    c->size = sizeof(gl_command_t);
    send_data(GLSC_BREAK, (void*)c, sizeof(gl_command_t));

}
void glSwapBuffer(){
    gl_command_t* c = (gl_command_t*) malloc(sizeof(gl_command_t));
    c->cmd = GLSC_bufferSwap;
    c->size = sizeof(gl_command_t);
    send_data(GLSC_bufferSwap, (void*)c, sizeof(gl_command_t));

}
unsigned int glCreateShader(unsigned int type){
    GL_SET_COMMAND(c, glCreateShader);
    c->cmd = GLSC_glCreateShader;
    c->type = type;
    unsigned int* ret =  (unsigned int *)send_data(GLSC_glCreateShader, (void*)c, sizeof(gl_glCreateShader_t));
    return (unsigned int) *ret;
}

void glShaderSource(int shader, int count, const char** string, const int* length){
    GL_SET_COMMAND(c, glShaderSource);
    c->cmd = GLSC_glShaderSource;
    c->shader = shader;
    c->count = count;
    c->string = string;
    c->length = length;
    send_data(GLSC_glShaderSource, (void*)c, sizeof(gl_glShaderSource_t));
    //SEND_MORE 
}
void glCompileShader(int shader){
    GL_SET_COMMAND(c, glCompileShader);
    c->cmd = GLSC_glCompileShader;
    c->shader = shader;
    send_data(GLSC_glCompileShader, (void*)c, sizeof(gl_glCompileShader_t));
}

void glGetShaderiv(int shader, unsigned int pname, int* param){
    GL_SET_COMMAND(c, glGetShaderiv);
    c->cmd = GLSC_glGetShaderiv;
    c->shader = shader;
    c->pname = pname;
    int* ret = (int*) send_data(GLSC_glGetShaderiv, (void*)c, sizeof(gl_glGetShaderiv_t));
    memcpy((void *)param, (void *) ret, sizeof(unsigned int));

}

unsigned int glCreateProgram(){
    GL_SET_COMMAND(c, glCreateProgram);
    c->cmd = GLSC_glCreateProgram;
    unsigned int* ret =  (unsigned int *)send_data(GLSC_glCreateProgram, (void*)c, sizeof(gl_glCreateProgram_t));
    return (unsigned int) *ret;
}

void glAttachShader(unsigned int program, int shader){
    GL_SET_COMMAND(c, glAttachShader);
    c->cmd = GLSC_glAttachShader;
    c->program = program;
    c->shader = shader;
    send_data(GLSC_glAttachShader, (void*)c, sizeof(gl_glAttachShader_t));
}

void glLinkProgram(unsigned int program){
    GL_SET_COMMAND(c, glLinkProgram);
    c->cmd = GLSC_glLinkProgram;
    c->program = program;
    send_data(GLSC_glLinkProgram, (void*)c, sizeof(gl_glLinkProgram_t));
}

void glGetProgramiv(int program, unsigned int pname, int* param){
    GL_SET_COMMAND(c, glGetProgramiv);
    c->cmd = GLSC_glGetProgramiv;
    c->program = program;
    c->pname = pname;
    int* ret  = (int*) send_data(GLSC_glGetProgramiv, (void*)c, sizeof(gl_glGetProgramiv_t));
    // param  = (int*) send_data(GLSC_glGetProgramiv, (void*)c, sizeof(gl_glGetProgramiv_t));
    memcpy((void *)param, (void *) ret, sizeof(unsigned int));

}

void glGenBuffers(int n, unsigned int* buffers){
    GL_SET_COMMAND(c, glGenBuffers);
    c->cmd = GLSC_glGenBuffers;
    c->n = n;
    // buffers = (unsigned int*) send_data(GLSC_glGenBuffers, (void*)c, sizeof(gl_glGenBuffers_t));

    unsigned int* ret  = (unsigned int*) send_data(GLSC_glGenBuffers, (void*)c, sizeof(gl_glGenBuffers_t));
    memcpy((void *)buffers, (void *) ret, sizeof(unsigned int));
}

void glBindBuffer(unsigned int target, unsigned int id){
    GL_SET_COMMAND(c, glBindBuffer);
    c->cmd = GLSC_glBindBuffer;
    c->target = target;
    c->id = id;
    send_data(GLSC_glBindBuffer, (void*)c, sizeof(gl_glBindBuffer_t));
}

void glBufferData(unsigned int target, long int size, const void* data, unsigned int usage){
    GL_SET_COMMAND(c, glBufferData);
    c->cmd = GLSC_glBufferData;
    c->target = target;
    c->size = size;
    c->data = data;
    c->usage = usage;
    send_data(GLSC_glBufferData, (void*)c, sizeof(gl_glBufferData_t));
}

void glGenVertexArrays(int n, const unsigned int* arrays){
    GL_SET_COMMAND(c, glGenVertexArrays);
    c->cmd = GLSC_glGenVertexArrays;
    c->n = n;
    // buffers = (unsigned int*) send_data(GLSC_glGenBuffers, (void*)c, sizeof(gl_glGenBuffers_t));

    unsigned int* ret  = (unsigned int*) send_data(GLSC_glGenVertexArrays, (void*)c, sizeof(gl_glGenVertexArrays_t));
    memcpy((void *)arrays, (void *) ret, sizeof(unsigned int));
}

void glBindVertexArray(unsigned int array){
    GL_SET_COMMAND(c, glBindVertexArray);
    c->cmd = GLSC_glBindVertexArray;
    c->array = array;
    send_data(GLSC_glBindVertexArray, (void*)c, sizeof(gl_glBindVertexArray_t));
}


int glGetAttribLocation(unsigned int programObj, const char* name){
    GL_SET_COMMAND(c, glGetAttribLocation);
    c->cmd = GLSC_glGetAttribLocation;
    c->programObj = programObj;
    c->name = name;
    int* ret =  (int *)send_data(GLSC_glGetAttribLocation, (void*)c, sizeof(gl_glGetAttribLocation_t));
    return (int) *ret;
}
void glVertexAttribPointer(unsigned int index, int size, unsigned int type, bool normalized, int stride, const void *pointer){
    GL_SET_COMMAND(c, glVertexAttribPointer);
    c->cmd = GLSC_glVertexAttribPointer;
    c->index = index;
    c->size = size;
    c->type = type;
    c->normalized = normalized;
    c->stride = stride;
    c->pointer = pointer;
    send_data(GLSC_glVertexAttribPointer, (void*)c, sizeof(gl_glVertexAttribPointer_t));
}

void glEnableVertexAttribArray(unsigned int index){
    GL_SET_COMMAND(c, glEnableVertexAttribArray);
    c->cmd = GLSC_glEnableVertexAttribArray;
    c->index = index;
    send_data(GLSC_glEnableVertexAttribArray, (void*)c, sizeof(gl_glEnableVertexAttribArray_t));
}
void glUseProgram(unsigned int program){
    GL_SET_COMMAND(c, glUseProgram);
    c->cmd = GLSC_glUseProgram;
    c->program = program;
    send_data(GLSC_glUseProgram, (void*)c, sizeof(gl_glUseProgram_t));
}

void glClearColor(float red, float green, float blue, float alpha){
    GL_SET_COMMAND(c, glClearColor);
    c->cmd = GLSC_glClearColor;
    c->red = red;
    c->green = green;
    c->blue = blue;
    c->alpha = alpha;
    send_data(GLSC_glClearColor, (void*)c, sizeof(gl_glClearColor_t));
}
void glDrawArrays(unsigned int mode, int first, int count){
    GL_SET_COMMAND(c, glDrawArrays);
    c->cmd = GLSC_glDrawArrays;
    c->mode = mode;
    c->first = first;
    c->count = count;
    send_data(GLSC_glDrawArrays, (void*)c, sizeof(gl_glDrawArrays_t));
}
int main(int argc, char* argv[]) {  
    // sock2.connect("tcp://127.0.0.1:12345");
    sock2.connect("tcp://" + std::string(argv[1]) + ":" + std::string(argv[2]));

    // shader initialization
    int result;
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader); 
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    
    // link shader program
    unsigned int triangleShaderProgramID;
    triangleShaderProgramID = glCreateProgram();

    glAttachShader(triangleShaderProgramID, vertexShader);
	glAttachShader(triangleShaderProgramID, fragmentShader);
	glLinkProgram(triangleShaderProgramID);	
    glGetProgramiv(triangleShaderProgramID, GL_LINK_STATUS, &result);
    // bind vbo
    unsigned int triangleVertexArrayObject;
    unsigned int trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
    glGenBuffers(1, &trianglePositionVertexBufferObjectID);
    glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW); 

	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

    //vertex arrays
    glGenVertexArrays(1, &triangleVertexArrayObject);
	glBindVertexArray(triangleVertexArrayObject);

    int positionAttribute;
    positionAttribute = glGetAttribLocation(triangleShaderProgramID, "positionAttribute");

	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	int colorAttribute = glGetAttribLocation(triangleShaderProgramID, "colorAttribute");
	
    glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorAttribute);

	glBindVertexArray(0);

	glUseProgram(triangleShaderProgramID);
	glBindVertexArray(triangleVertexArrayObject);

    while (1){
        glClearColor(0, 0, 0, 1); 
        glClear(GL_COLOR_BUFFER_BIT); 


        glDrawArrays(GL_TRIANGLES, 0, 3);
        glSwapBuffer();
    }
    return 0;
}

