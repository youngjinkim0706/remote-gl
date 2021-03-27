#include "khrplatform.h"


#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82


#define GL_TRIANGLES 0x0004

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_POLYGON 0x0009

#define GL_ARRAY_BUFFER 0x8892

#define GL_STATIC_DRAW 0x88E4

#define GL_FLOAT 0x1406
#define GL_FALSE 0

#ifndef GL_APICALL
#define GL_APICALL  KHRONOS_APICALL
#endif

#ifndef GL_APIENTRY
#define GL_APIENTRY KHRONOS_APIENTRY
#endif

GL_APICALL void GL_APIENTRY glClear (unsigned int mask);
GL_APICALL void GL_APIENTRY glBegin (unsigned int mode);
GL_APICALL void GL_APIENTRY glColor3f(float red, float green, float blue);
GL_APICALL void GL_APIENTRY glVertex3f(float x, float y, float z);
GL_APICALL void GL_APIENTRY glEnd();
GL_APICALL void GL_APIENTRY glFlush();
GL_APICALL unsigned int GL_APIENTRY glCreateShader(unsigned int type);
GL_APICALL void GL_APIENTRY glShaderSource(int shader, int count, const char *const* string, const int* length);
GL_APICALL void GL_APIENTRY glCompileShader (int shader);
GL_APICALL void GL_APIENTRY glGetShaderiv (int shader, unsigned int pname, int* param);
GL_APICALL unsigned int GL_APIENTRY glCreateProgram();
GL_APICALL void GL_APIENTRY glAttachShader(unsigned int program, int shader);
GL_APICALL void GL_APIENTRY glLinkProgram(unsigned int program);
GL_APICALL void GL_APIENTRY glGetProgramiv (int program, unsigned int pname, int* param);
GL_APICALL void GL_APIENTRY glGenBuffers (int n, unsigned int* buffers);
GL_APICALL void GL_APIENTRY glBindBuffer(unsigned int target, unsigned int id);
GL_APICALL void GL_APIENTRY glBufferData(unsigned int target, long int size, const void* data, unsigned int usage);
GL_APICALL void GL_APIENTRY glGenVertexArrays (int n, const unsigned int* arrays);
GL_APICALL void GL_APIENTRY glBindVertexArray(unsigned int array);
GL_APICALL int GL_APIENTRY glGetAttribLocation(unsigned int programObj, const char* name);
GL_APICALL void GL_APIENTRY glVertexAttribPointer(unsigned int index, int size, unsigned int type, bool normalized, int stride, const void *pointer);
GL_APICALL void GL_APIENTRY glEnableVertexAttribArray(unsigned int index);
GL_APICALL void GL_APIENTRY glUseProgram(unsigned int program);
GL_APICALL void GL_APIENTRY glClearColor (float red, float green, float blue, float alpha);
GL_APICALL void GL_APIENTRY glDrawArrays (unsigned int mode, int first, int count);
