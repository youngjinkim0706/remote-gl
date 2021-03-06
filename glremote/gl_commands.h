#include "glad/glad.h"
#include <stdint.h>
// add all godot gl command
enum GL_Server_Command
{
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
    GLSC_glViewport,
    GLSC_glScissor,
    GLSC_glGetIntegerv,
    GLSC_glGetFloatv,
    GLSC_glGenTextures,
    GLSC_glActiveTexture,
    GLSC_glBindTexture,
    GLSC_glTexImage2D,
    GLSC_glGetError,
    GLSC_glTexStorage2D,
    GLSC_glTexParameteri,
    GLSC_glGenFramebuffers,
    GLSC_glBindFramebuffer,
    GLSC_glFramebufferTexture2D,
    GLSC_glCheckFramebufferStatus,
    GLSC_glDisable,
    GLSC_glEnable,
    GLSC_glTexImage3D,
    GLSC_glGenerateMipmap,
    GLSC_glFrontFace,
    GLSC_glDepthMask,
    GLSC_glBlendEquation,
    GLSC_glBlendFunc,
    GLSC_glVertexAttrib4f,
    GLSC_glUniform1i,
    GLSC_glUniformBlockBinding,
    GLSC_glPixelStorei,
    GLSC_glTexParameterf,
    GLSC_glUniform4fv,
    GLSC_glUniformMatrix4fv,
    GLSC_glUniform2fv,
    GLSC_glBindBufferBase,
    GLSC_glColorMask,
    GLSC_glDepthFunc,
    GLSC_glGenRenderbuffers,
    GLSC_glBindRenderbuffer,
    GLSC_glRenderbufferStorage,
    GLSC_glFramebufferRenderbuffer,
    GLSC_glTexSubImage3D,
    GLSC_glDeleteTextures,
    GLSC_glDeleteFramebuffers,
    GLSC_glClearDepthf,
    GLSC_glClearBufferfv,
    GLSC_glGetStringi,
    GLSC_glTexSubImage2D,
    GLSC_glReadPixels,
    GLSC_glBlendFuncSeparate,
    GLSC_glDisableVertexAttribArray,
    GLSC_glVertexAttrib4fv,
    GLSC_glDrawElements,
    GLSC_glReadBuffer,
    GLSC_glBlitFramebuffer,
    GLSC_glDeleteRenderbuffers,
    GLSC_glUniform1f,
    GLSC_glFramebufferTextureLayer,
    GLSC_glRenderbufferStorageMultisample,
    GLSC_glLoadIdentity,
    GLSC_glMatrixMode,
    GLSC_glRotatef,
    GLSC_glOrtho,
};

typedef struct
{
    unsigned int cmd;
    GLenum cap;
} gl_glDisable_t, gl_glEnable_t;

typedef struct
{
    unsigned int cmd;
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
} gl_glOrtho_t;

typedef struct
{
    unsigned int cmd;
    GLenum func;
} gl_glDepthFunc_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLint levels;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
} gl_glTexStorage2D_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum pname;
    GLint param;
} gl_glTexParameteri_t;

typedef struct
{
    unsigned int cmd;
    GLfloat angle;
    GLfloat x;
    GLfloat y;
    GLfloat z;
} gl_glRotatef_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
} gl_glRenderbufferStorage_t;

typedef struct
{
    unsigned int cmd;
    GLint srcX0;
    GLint srcY0;
    GLint srcX1;
    GLint srcY1;
    GLint dstX0;
    GLint dstY0;
    GLint dstX1;
    GLint dstY1;
    GLbitfield mask;
    GLenum filter;
} gl_glBlitFramebuffer_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum attachment;
    GLuint texture;
    GLint level;
    GLint layer;
} gl_glFramebufferTextureLayer_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLsizei samples;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
} gl_glRenderbufferStorageMultisample_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
} gl_glTexImage2D_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    const void *pixels;
} gl_glTexSubImage2D_t;

typedef struct
{
    unsigned int cmd;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    void *pixels;
} gl_glReadPixels_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLint level;
    GLint internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    const void *pixels;
} gl_glTexImage3D_t;

typedef struct
{
    unsigned int cmd;
    GLsizei n;
    const GLuint *textures;
} gl_glDeleteTextures_t;

typedef struct
{
    unsigned int cmd;
    GLsizei n;
    const GLuint *framebuffers;
} gl_glDeleteFramebuffers_t;

typedef struct
{
    unsigned int cmd;
    GLsizei n;
    const GLuint *renderbuffers;
} gl_glDeleteRenderbuffers_t;

typedef struct
{
    unsigned int cmd;
    GLenum buffer;
    GLint drawbuffer;
    const GLfloat *value;
} gl_glClearBufferfv_t;

typedef struct
{
    unsigned int cmd;
    GLfloat d;
} gl_glClearDepthf_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    const void *pixels;
} gl_glTexSubImage3D_t;

typedef struct
{
    unsigned int cmd;
    GLenum pname;
    GLint *data;
} gl_glGetIntegerv_t;

typedef struct
{
    unsigned int cmd;
    GLenum pname;
    GLfloat *data;
} gl_glGetFloatv_t;

typedef struct
{
    unsigned int cmd;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
} gl_glScissor_t;

typedef struct
{
    unsigned int cmd;
    GLuint program;
    GLuint uniformBlockIndex;
    GLuint uniformBlockBinding;
} gl_glUniformBlockBinding_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum attachment;
    GLenum renderbuffertarget;
    GLuint renderbuffer;
} gl_glFramebufferRenderbuffer_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLuint index;
    GLuint buffer;
} gl_glBindBufferBase_t;

typedef struct
{
    unsigned int cmd;
    GLboolean red;
    GLboolean green;
    GLboolean blue;
    GLboolean alpha;
} gl_glColorMask_t;

typedef struct
{
    unsigned int cmd;
    GLenum pname;
    GLint param;
} gl_glPixelStorei_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum pname;
    GLfloat param;
} gl_glTexParameterf_t;

typedef struct
{
    unsigned int cmd;
    int x;
    int y;
    int width;
    int height;
} gl_glViewport_t;

typedef struct
{
    unsigned int cmd;
    int size;
} gl_command_t, gl_glLoadIdentity_t;

typedef struct
{
    unsigned int cmd;
    GLenum name;
    GLuint index;
} gl_glGetStringi_t;

typedef struct
{
    unsigned int cmd;
    GLuint index;
} gl_glDisableVertexAttribArray_t;

typedef struct
{
    unsigned int cmd;
} gl_glFlush_t, gl_glCreateProgram_t, gl_glEnd_t, gl_glBreak_t, gl_glGetError_t;

typedef struct
{
    unsigned int cmd;
    unsigned int mask;
} gl_glClear_t;

typedef struct
{
    unsigned int cmd;
    GLint location;
    GLint v0;
} gl_glUniform1i_t;

typedef struct
{
    unsigned int cmd;
    GLint location;
    GLfloat v0;
} gl_glUniform1f_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
} gl_glGenerateMipmap_t;

typedef struct
{
    unsigned int cmd;
    GLenum mode;
} gl_glBegin_t, gl_glFrontFace_t, gl_glBlendEquation_t, gl_glMatrixMode_t;

typedef struct
{
    unsigned int cmd;
    GLenum sfactor;
    GLenum dfactor;
} gl_glBlendFunc_t;

typedef struct
{
    unsigned int cmd;
    GLenum sfactorRGB;
    GLenum dfactorRGB;
    GLenum sfactorAlpha;
    GLenum dfactorAlpha;
} gl_glBlendFuncSeparate_t;

typedef struct
{
    unsigned int cmd;
    float red;
    float green;
    float blue;
} gl_glColor3f_t;

typedef struct
{
    unsigned int cmd;
    float x;
    float y;
    float z;
} gl_glVertex3f_t;

typedef struct
{
    unsigned int cmd;
    GLuint index;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
} gl_glVertexAttrib4f_t;

typedef struct
{
    unsigned int cmd;
    GLuint index;
    const GLfloat *v;
} gl_glVertexAttrib4fv_t;

typedef struct
{
    unsigned int cmd;
    GLenum mode;
    GLsizei count;
    GLenum type;
    const void *indices;
} gl_glDrawElements_t;

typedef struct
{
    unsigned int cmd;
    unsigned int type;
} gl_glCreateShader_t;

typedef struct
{
    unsigned int cmd;
    GLenum src;
} gl_glReadBuffer_t;

typedef struct
{
    unsigned int cmd;
    int shader;
    int count;
    const char *const *string;
    const int *length; // usally null
} gl_glShaderSource_t;

typedef struct
{
    unsigned int cmd;
    int shader;
} gl_glCompileShader_t;

typedef struct
{
    unsigned int cmd;
    GLboolean flag;
} gl_glDepthMask_t;

typedef struct
{
    unsigned int cmd;
    int shader;
    unsigned int pname;
    int *result;
} gl_glGetShaderiv_t;

typedef struct
{
    unsigned int cmd;
    int program;
    int shader;
} gl_glAttachShader_t;

typedef struct
{
    unsigned int cmd;
    unsigned int program;
} gl_glLinkProgram_t, gl_glUseProgram_t;

typedef struct
{
    unsigned int cmd;
    int program;
    unsigned int pname;
    int *result;
} gl_glGetProgramiv_t;

typedef struct
{
    unsigned int cmd;
    int n;
    GLuint *buffers;
} gl_glGenBuffers_t, gl_glGenFramebuffers_t, gl_glGenRenderbuffers_t;

typedef struct
{
    unsigned int cmd;
    int n;
    GLuint *buffers;
} gl_glGenTextures_t;

typedef struct
{
    unsigned int cmd;
    GLenum texture;
} gl_glActiveTexture_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLuint texture;
} gl_glBindTexture_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLuint framebuffer;
} gl_glBindFramebuffer_t;

typedef struct
{
    unsigned int cmd;
    GLint location;
    GLsizei count;
    const GLfloat *value;

} gl_glUniform4fv_t, gl_glUniform2fv_t;
typedef struct
{
    unsigned int cmd;
    GLint location;
    GLsizei count;
    GLboolean transpose;
    const GLfloat *value;

} gl_glUniformMatrix4fv_t;
typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLenum attachment;
    GLenum textarget;
    GLuint texture;
    GLint level;
} gl_glFramebufferTexture2D_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
} gl_glCheckFramebufferStatus_t;

typedef struct
{
    unsigned int cmd;
    unsigned int target;
    unsigned int id;
} gl_glBindBuffer_t;

typedef struct
{
    unsigned int cmd;
    GLenum target;
    GLuint renderbuffer;
} gl_glBindRenderbuffer_t;

typedef struct
{
    unsigned int cmd;
    unsigned int target;
    long int size;
    const void *data;
    unsigned int usage;
} gl_glBufferData_t;

typedef struct
{
    unsigned int cmd;
    GLsizei n;
    GLuint *arrays;
} gl_glGenVertexArrays_t;

typedef struct
{
    unsigned int cmd;
    unsigned int array;
} gl_glBindVertexArray_t;

typedef struct
{
    unsigned int cmd;
    unsigned int programObj;
    const char *name;
} gl_glGetAttribLocation_t;

typedef struct
{
    unsigned int cmd;
    unsigned int index;
    int size;
    unsigned int type;
    bool normalized;
    int stride;
    const void *pointer;
} gl_glVertexAttribPointer_t;

typedef struct
{
    unsigned int cmd;
    unsigned int index;
} gl_glEnableVertexAttribArray_t;

typedef struct
{
    unsigned int cmd;
    float red;
    float green;
    float blue;
    float alpha;
} gl_glClearColor_t;

typedef struct
{
    unsigned int cmd;
    unsigned int mode;
    int first;
    int count;
} gl_glDrawArrays_t;
