#include <GL/glew.h>		
#include <GLFW/glfw3.h> 
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

using namespace std;


int framebufferWidth, framebufferHeight;
GLuint triangleVertexArrayObject;
GLuint triangleShaderProgramID;
GLuint trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;


bool initShaderProgram() {

	//#3
	const GLchar* vertexShaderSource = 
		"#version 330 core\n"
		"in vec3 positionAttribute;"
		"in vec3 colorAttribute;"
		"out vec3 passColorAttribute;"
		"void main()"
		"{"
		"gl_Position = vec4(positionAttribute, 1.0);"
		"passColorAttribute = colorAttribute;"
		"}";


	//#4
	const GLchar* fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 passColorAttribute;"
		"out vec4 fragmentColor;"
		"void main()"
		"{"
		"fragmentColor = vec4(passColorAttribute, 1.0);"
		"}";



	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << endl;
		glDeleteShader(vertexShader);
		return false;
	}


	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader); 

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << endl;

		return false;
	}




	//#5
	triangleShaderProgramID = glCreateProgram();
    // std::cout << triangleShaderProgramID << std::endl;

	glAttachShader(triangleShaderProgramID, vertexShader);
	glAttachShader(triangleShaderProgramID, fragmentShader);

	glLinkProgram(triangleShaderProgramID);


	// glDeleteShader(vertexShader);
	// glDeleteShader(fragmentShader);


	glGetProgramiv(triangleShaderProgramID, GL_LINK_STATUS, &result);
	// std::cout << result << std::endl;
	if (!result) {
		glGetProgramInfoLog(triangleShaderProgramID, 512, NULL, errorLog);
		cerr << "ERROR: shader program 연결 실패\n" << errorLog << endl;
		return false;
	}

	return true;
}



bool defineVertexArrayObject() {

	//#1
	//삼각형을 구성하는 vertex 데이터 - position과 color
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



	//#2
	//Vertex Buffer Object(VBO)를 생성하여 vertex 데이터를 복사한다.
	glGenBuffers(1, &trianglePositionVertexBufferObjectID);
	// std::cout << trianglePositionVertexBufferObjectID << std::endl;
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW); 

	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);



	//#6
	glGenVertexArrays(1, &triangleVertexArrayObject);
	glBindVertexArray(triangleVertexArrayObject);


	GLint positionAttribute = glGetAttribLocation(triangleShaderProgramID, "positionAttribute");
	if (positionAttribute == -1) {
		cerr << "position 속성 설정 실패" << endl;
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(positionAttribute);

	GLint colorAttribute = glGetAttribLocation(triangleShaderProgramID, "colorAttribute");
	if (colorAttribute == -1) {
		cerr << "color 속성 설정 실패" << endl;
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorAttribute);


	glBindVertexArray(0);


	return true;
}




void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	//처음 2개의 파라미터는 viewport rectangle의 왼쪽 아래 좌표
	//다음 2개의 파라미터는 viewport의 너비와 높이이다.
	//framebuffer의 width와 height를 가져와 glViewport에서 사용한다.
	glViewport(0, 0, width, height);

	framebufferWidth = width;
	framebufferHeight = height;
}



void errorCallback(int errorCode, const char* errorDescription) 
{
	cerr << "Error: " << errorDescription << endl;
}


 
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) 
{  
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);  
}




int main()
{
	 
	glfwSetErrorCallback(errorCallback);   
	 

	if (!glfwInit()) { 	 

		cerr << "Error: GLFW 초기화 실패" << endl;
		std::exit(EXIT_FAILURE);
	}



	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);


	GLFWwindow* window = glfwCreateWindow( 
		800, 
		600, 
		"OpenGL Example", 
		NULL, NULL);
	if (!window) {

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}


	glfwMakeContextCurrent(window);


	glfwSetKeyCallback(window, keyCallback); 
	// glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);



	glewExperimental = GL_TRUE;
	GLenum errorCode = glewInit();  
	if (GLEW_OK != errorCode) {

		cerr << "Error: GLEW 초기화 실패 - " << glewGetErrorString(errorCode) << endl;

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}



	if (!GLEW_VERSION_3_3) {

		cerr << "Error: OpenGL 3.3 API is not available." << endl;

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}


	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl; 
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;




	if (!initShaderProgram()) {

		cerr << "Error: Shader Program 생성 실패" << endl;

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}



	if (!defineVertexArrayObject()) {

		cerr << "Error: Shader Program 생성 실패" << endl;

		glfwTerminate();
		std::exit(EXIT_FAILURE);
	}




	
	glfwSwapInterval(1);

	

	double lastTime = glfwGetTime(); 
	int numOfFrames = 0;
	int count = 0;





	glUseProgram(triangleShaderProgramID);
	glBindVertexArray(triangleVertexArrayObject);


	while (!glfwWindowShouldClose(window)) {  


		double currentTime = glfwGetTime();
		numOfFrames++;
		if (currentTime - lastTime >= 1.0) { 

			printf("%f ms/frame  %d fps \n", 1000.0 / double(numOfFrames), numOfFrames);
			numOfFrames = 0;
			lastTime = currentTime;
		}



		glClearColor(0, 0, 0, 1); 
		glClear(GL_COLOR_BUFFER_BIT); 


		glDrawArrays(GL_TRIANGLES, 0, 3);


		count++;

		unsigned char *data  = (unsigned char*) malloc(800 * 600 * 4); // GL_RGBA

		glfwSwapBuffers(window); 
		// glReadBuffer(GL_FRONT);
		// glReadPixels(0, 0, 800, 600, GL_RGBA, GL_UNSIGNED_BYTE, data);

		// stbi_flip_vertically_on_write(true);
		// stbi_write_jpg("jpg_test.jpg", 800, 600, 4, data, 4 * 800);
		glfwPollEvents(); 

	}


	glUseProgram(0);
	glBindVertexArray(0);


	glDeleteProgram(triangleShaderProgramID);
	glDeleteBuffers(1, &trianglePositionVertexBufferObjectID);
	glDeleteBuffers(1, &triangleColorVertexBufferObjectID);
	glDeleteVertexArrays(1, &triangleVertexArrayObject);
	glfwTerminate();  

	std::exit(EXIT_SUCCESS);
}