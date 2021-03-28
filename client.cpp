#include <zmq.hpp>
#include <string>
#include <iostream>
// #include <GL/glew.h>
#include "glremote/glremote/glremote.h"
#include "glremote/gl_commands.h"

#define GL_SET_COMMAND(PTR, FUNCNAME) gl_##FUNCNAME##_t *PTR = (gl_##FUNCNAME##_t *)malloc(sizeof(gl_##FUNCNAME##_t)); PTR->cmd = GLSC_##FUNCNAME

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

// zmq::context_t ctx2;
// zmq::socket_t sock2(ctx2, zmq::socket_type::req);

int main(int argc, char* argv[]) {  
    // sock2.connect("tcp://127.0.0.1:12345");
    // sock2.connect("tcp://" + std::string(argv[1]) + ":" + std::string(argv[2]));

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
        std::cout << sizeof(gl_glClearColor_t) + sizeof(gl_glClear_t) + sizeof(gl_glDrawArrays_t) << std::endl;
        glSwapBuffer();
    }
    return 0;
}

