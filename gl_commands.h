#include <stdint.h>

// add all godot gl command
enum GL_Server_Command{
  GLSC_BREAK,
  GLSC_bufferSwap,
  GLSC_glClear,
  GLSC_glBegin,
  GLSC_glColor3f,
  GLSC_glVertex3f,
  GLSC_glEnd,
  GLSC_glFlush,
  GLSC_glCreateShader,
  GLSC_glShaderSource,
  GLSC_glCompileShader,
  GLSC_glGetShaderiv,
  GLSC_glCreateProgram,
  GLSC_glAttachShader,
  GLSC_glLinkProgram,
  GLSC_glGetProgramiv,
  GLSC_glGenBuffers,
  GLSC_glBindBuffer,
  GLSC_glBufferData,
  GLSC_glGenVertexArrays,
  GLSC_glBindVertexArray,
  GLSC_glGetAttribLocation,
  GLSC_glVertexAttribPointer,
  GLSC_glEnableVertexAttribArray,
  GLSC_glUseProgram,
  GLSC_glClearColor,
  GLSC_glDrawArrays,
};

typedef struct{
  unsigned int cmd;
  int size;
  // void* cmd_data;
} gl_command_t;

typedef struct {
  unsigned int cmd;
} gl_glFlush_t, gl_glCreateProgram_t, gl_glEnd_t, gl_glBreak_t;

typedef struct{
  unsigned int cmd;
  unsigned int mask;
} gl_glClear_t;

typedef struct{
  unsigned int cmd;
  unsigned int mode;
} gl_glBegin_t;

typedef struct{
  unsigned int cmd;
  float red;
  float green;
  float blue;
} gl_glColor3f_t;

typedef struct{
  unsigned int cmd;
  float x;
  float y;
  float z;
} gl_glVertex3f_t;

typedef struct {
  unsigned int cmd;
  unsigned int type;
} gl_glCreateShader_t;

typedef struct {
  unsigned int cmd;
  int shader;
  int count;
  const char** string;
  const int* length; // usally null
} gl_glShaderSource_t;

typedef struct {
  unsigned int cmd;
  int shader;
} gl_glCompileShader_t;

typedef struct {
  unsigned int cmd;
  int shader;
  unsigned int pname;
  int* result;
} gl_glGetShaderiv_t;

typedef struct {
  unsigned int cmd;
  int program;
  int shader;
} gl_glAttachShader_t;

typedef struct {
  unsigned int cmd;
  unsigned int program;
} gl_glLinkProgram_t, gl_glUseProgram_t;

typedef struct {
  unsigned int cmd;
  int program;
  unsigned int pname;
  int* result;
} gl_glGetProgramiv_t;

typedef struct {
  unsigned int cmd;
  int n;
  unsigned int* buffers;
} gl_glGenBuffers_t;

typedef struct {
  unsigned int cmd;
  unsigned int target;
  unsigned int id;
} gl_glBindBuffer_t;

typedef struct {
  unsigned int cmd;
  unsigned int target;
  long int size;
  const void* data;
  unsigned int usage;
} gl_glBufferData_t;

typedef struct {
  unsigned int cmd;
  int n;
  const unsigned int* arrays;
} gl_glGenVertexArrays_t;

typedef struct {
  unsigned int cmd;
  unsigned int array;
} gl_glBindVertexArray_t;

typedef struct {
  unsigned int cmd;
  unsigned int programObj;
  const char* name;
} gl_glGetAttribLocation_t;

typedef struct {
  unsigned int cmd;
  unsigned int index;
  int size;
  unsigned int type;
  bool normalized;
  int stride;
  const void* pointer;
} gl_glVertexAttribPointer_t;

typedef struct{
  unsigned int cmd;
  unsigned int index;
} gl_glEnableVertexAttribArray_t;

typedef struct{
  unsigned int cmd;
  float red;
  float green;
  float blue;
  float alpha;
} gl_glClearColor_t;

typedef struct{
  unsigned int cmd;
  unsigned int mode;
  int first;
  int count;
} gl_glDrawArrays_t;

