#include "glremote_server.h"
#include <vector>

int Server::framebufferHeight = 0;
int Server::framebufferWidth = 0;
int shader_compiled = 0;

unsigned int total_data_size = 0;

static void errorCallback(int errorCode, const char *errorDescription)

{
  fprintf(stderr, "Error: %s\n", errorDescription);
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {

  if (type == GL_DEBUG_TYPE_ERROR) {
    fprintf(stderr,
            "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type,
            severity, message);
    exit(-1);
  }
}
cache_key Server::cache_key_gen(unsigned char cmd, int sequence_number) {
  std::bitset<32> cmd_bit(cmd);
  std::bitset<32> seq_bit(sequence_number);
  cmd_bit <<= 24;
  return (cmd_bit |= seq_bit).to_ulong();
}

std::string Server::get_value_from_request(zmq::message_t &msg) {
  std::string value;
  value.resize(msg.size());
  memcpy((void *)value.data(), msg.data(), msg.size());
  return value;
}

std::string Server::alloc_cached_data(zmq::message_t &data_msg) {
  std::string cache_data;
  if(!data_msg.empty()){
    cache_data.resize(data_msg.size());
    memcpy((void *)cache_data.data(), data_msg.data(), data_msg.size());
  }
  return cache_data;
}

std::string
Server::insert_or_check_cache(std::map<cache_key, std::string> &cache,
                              cache_key key, zmq::message_t &data_msg) {
  bool cached = false;
  std::string cache_data = alloc_cached_data(data_msg);
  auto res = cache.insert(std::make_pair(key, cache_data));
  
  if (cache_data.empty()){
    if (!res.second){
        cached = true;
        // total_data_size += res.first->second.size();
        return res.first->second;        
    }
  }
  total_data_size += cache_data.size();
  return cache_data;
}
void Server::server_bind() {
  sock = zmq::socket_t(ctx, zmq::socket_type::pair);

  sock.bind("tcp://" + ip_address + ":" + port);

  if (enableStreaming) {
    boost::interprocess::message_queue::remove(streaming_queue_name.c_str());
    std::cout << streaming_queue_name.c_str() << std::endl;
    size_t max_msg_size = WIDTH * HEIGHT * 4;
    mq.reset(new boost::interprocess::message_queue(
        boost::interprocess::create_only, streaming_queue_name.c_str(), 1024,
        max_msg_size));
  }
}

static void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  //처음 2개의 파라미터는 viewport rectangle의 왼쪽 아래 좌표
  //다음 2개의 파라미터는 viewport의 너비와 높이이다.
  // framebuffer의 width와 height를 가져와 glViewport에서 사용한다.
  glViewport(0, 0, width, height);

  Server::framebufferWidth = width;
  Server::framebufferHeight = height;
}

void Server::init_gl() {

  glfwSetErrorCallback(errorCallback);
  if (!glfwInit()) {
    std::cerr << "Error: GLFW 초기화 실패" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // glfwWindowHint(GLFW_SAMPLES, 4);
  // glfwWindowHint(GLFW_VISIBLE, GL_TRUE);

  window =
      glfwCreateWindow(WIDTH, HEIGHT, "Remote Drawing Example", NULL, NULL);

  if (!window) {
    glfwTerminate();
    std::exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  // glfwSwapInterval(1);
}
void Server::run() {
  bool quit = false;
  init_gl();

  double lastTime = glfwGetTime();
  int numOfFrames = 0;
  int count = 0;
  sequence_number = 0;

  while (!quit) {
    // waiting until data comes here
    zmq::message_t msg;
    zmq::message_t ret;
    bool hasReturn = false;

    auto res = sock.recv(msg, zmq::recv_flags::none);
    total_data_size += msg.size();

    gl_command_t *c = (gl_command_t *)msg.data();
    cache_key key = cache_key_gen((unsigned char)c->cmd, sequence_number);
    switch (c->cmd) {
    case (unsigned char)GL_Server_Command::GLSC_glClear: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glClear_t *cmd_data = (gl_glClear_t *)data.data();      
      glClear(cmd_data->mask);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBegin: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);      
      gl_glBegin_t *cmd_data = (gl_glBegin_t *)data.data();
      glBegin(cmd_data->mode);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glColor3f: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glColor3f_t *cmd_data = (gl_glColor3f_t *)data.data();
      glColor3f(cmd_data->red, cmd_data->green, cmd_data->blue);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glVertex3f: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glVertex3f_t *cmd_data = (gl_glVertex3f_t *)data.data();
      glVertex3f(cmd_data->x, cmd_data->y, cmd_data->z);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glEnd: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      glEnd();
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glFlush: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      glFlush();
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glCreateShader: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glCreateShader_t *cmd_data = (gl_glCreateShader_t *)data.data();
      GLuint shader = glCreateShader(cmd_data->type);

      hasReturn = true;
      ret.rebuild(sizeof(GLuint));
      memcpy(ret.data(), &shader, sizeof(GLuint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glShaderSource: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      
      gl_glShaderSource_t *cmd_data = (gl_glShaderSource_t *)data_msg.data();
      std::vector<const char *> strings;
      for (int i = 0; i < cmd_data->count; i++) {
        zmq::message_t more_data;
        auto res = sock.recv(more_data, zmq::recv_flags::none);
        if (!more_data.empty()) {

          std::string data(more_data.to_string());
          size_t size = data.size();

          char *data_str = new char[size + 1];
          strcpy(data_str, data.c_str());
          strings.push_back(data_str);

        } else {
          strings.push_back("\n");
        }
      }
      glShaderSource(cmd_data->shader, cmd_data->count, &strings[0], NULL);
      
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTransformFeedbackVaryings: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      
      gl_glTransformFeedbackVaryings_t *cmd_data =
          (gl_glTransformFeedbackVaryings_t *)data_msg.data();
      std::vector<const char *> strings;
      for (int i = 0; i < cmd_data->count; i++) {
        zmq::message_t more_data;
        auto res = sock.recv(more_data, zmq::recv_flags::none);
        if (!more_data.empty()) {

          std::string data(more_data.to_string());
          size_t size = data.size();

          char *data_str = new char[size + 1];
          strcpy(data_str, data.c_str());
          strings.push_back(data_str);

        } else {
          strings.push_back("\n");
        }
      }
      glTransformFeedbackVaryings(cmd_data->program, cmd_data->count,
                                  &strings[0], cmd_data->bufferMode);
      
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glCompileShader: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glCompileShader_t *cmd_data = (gl_glCompileShader_t *)data.data();
      glCompileShader(cmd_data->shader);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetShaderiv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetShaderiv_t *cmd_data = (gl_glGetShaderiv_t *)data.data();
      
      int result;
      glGetShaderiv(cmd_data->shader, cmd_data->pname, &result);
      
      shader_compiled++;
      if (!result) {
        GLchar errorLog[512];
        glGetShaderInfoLog(cmd_data->shader, 512, NULL, errorLog);
        std::cerr << "ERROR: vertex shader 컴파일 실패\n"
                  << errorLog << std::endl;
      }

      hasReturn = true;
      ret.rebuild(sizeof(int));
      memcpy(ret.data(), &result, sizeof(int));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glReadPixels: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glReadPixels_t *cmd_data = (gl_glReadPixels_t *)data.data();
      
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

      void *result = malloc(size);
      glReadPixels(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height,
                   cmd_data->format, cmd_data->type, result);

      hasReturn = true;
      ret.rebuild(size);

      memcpy(ret.data(), result, size);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBlendFuncSeparate: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBlendFuncSeparate_t *cmd_data =
          (gl_glBlendFuncSeparate_t *)data.data();
      glBlendFuncSeparate(cmd_data->sfactorRGB, cmd_data->dfactorRGB,
                          cmd_data->sfactorAlpha, cmd_data->dfactorAlpha);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBlendFunc: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBlendFunc_t *cmd_data = (gl_glBlendFunc_t *)data.data();
      glBlendFunc(cmd_data->sfactor, cmd_data->dfactor);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glVertexAttrib4f: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glVertexAttrib4f_t *cmd_data = (gl_glVertexAttrib4f_t *)data.data();

      glVertexAttrib4f(cmd_data->index, cmd_data->x, cmd_data->y, cmd_data->z,
                       cmd_data->w);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDisableVertexAttribArray: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDisableVertexAttribArray_t *cmd_data =
          (gl_glDisableVertexAttribArray_t *)data.data();
      glDisableVertexAttribArray(cmd_data->index);

      break;
    }

    case (unsigned char)GL_Server_Command::GLSC_glCreateProgram: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      GLuint program = glCreateProgram();

      hasReturn = true;
      ret.rebuild(sizeof(GLuint));
      memcpy(ret.data(), &program, sizeof(GLuint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glCheckFramebufferStatus: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glCheckFramebufferStatus_t *cmd_data =
          (gl_glCheckFramebufferStatus_t *)data.data();
      GLenum status = glCheckFramebufferStatus(cmd_data->target);

      hasReturn = true;
      ret.rebuild(sizeof(GLenum));
      memcpy(ret.data(), &status, sizeof(GLenum));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glAttachShader: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glAttachShader_t *cmd_data = (gl_glAttachShader_t *)data.data();
      glAttachShader(cmd_data->program, cmd_data->shader);

      break;
    }

    case (unsigned char)GL_Server_Command::GLSC_glDisable: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDisable_t *cmd_data = (gl_glDisable_t *)data.data();
      glDisable(cmd_data->cap);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexImage3D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexImage3D_t *cmd_data = (gl_glTexImage3D_t *)data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      if (!buffer_data.empty()) {
        glTexImage3D(cmd_data->target, cmd_data->level,
                     cmd_data->internalformat, cmd_data->width,
                     cmd_data->height, cmd_data->depth, cmd_data->border,
                     cmd_data->format, cmd_data->type, buffer_data.data());
      } else {
        glTexImage3D(cmd_data->target, cmd_data->level,
                     cmd_data->internalformat, cmd_data->width,
                     cmd_data->height, cmd_data->depth, cmd_data->border,
                     cmd_data->format, cmd_data->type, NULL);
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenerateMipmap: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);      
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenerateMipmap_t *cmd_data = (gl_glGenerateMipmap_t *)data.data();
      glGenerateMipmap(cmd_data->target);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glFrontFace: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glFrontFace_t *cmd_data = (gl_glFrontFace_t *)data.data();
      glFrontFace(cmd_data->mode);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDepthMask: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDepthMask_t *cmd_data = (gl_glDepthMask_t *)data.data();
      glDepthMask(cmd_data->flag);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBlendEquation: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBlendEquation_t *cmd_data = (gl_glBlendEquation_t *)data.data();
      glBlendEquation(cmd_data->mode);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glEnable: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glEnable_t *cmd_data = (gl_glEnable_t *)data.data();
      glEnable(cmd_data->cap);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glLinkProgram: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glLinkProgram_t *cmd_data = (gl_glLinkProgram_t *)data.data();
      glLinkProgram(cmd_data->program);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetProgramiv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetProgramiv_t *cmd_data = (gl_glGetProgramiv_t *)data.data();
      
      int result;
      glGetProgramiv(cmd_data->program, cmd_data->pname, &result);
      
      if (!result) {
        GLchar errorLog[512];
        glGetShaderInfoLog(cmd_data->program, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n"
                  << errorLog << std::endl;
      }

      hasReturn = true;
      ret.rebuild(sizeof(int));
      memcpy(ret.data(), &result, sizeof(int));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetError: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      GLenum error = glGetError();

      hasReturn = true;
      ret.rebuild(sizeof(GLenum));
      memcpy(ret.data(), &error, sizeof(GLenum));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexStorage2D: {
       // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexStorage2D_t *cmd_data = (gl_glTexStorage2D_t *)data.data();
      glTexStorage2D(cmd_data->target, cmd_data->levels,
                     cmd_data->internalformat, cmd_data->width,
                     cmd_data->height);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexParameteri: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexParameteri_t *cmd_data = (gl_glTexParameteri_t *)data.data();
      glTexParameteri(cmd_data->target, cmd_data->pname, cmd_data->param);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenBuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenBuffers_t *cmd_data = (gl_glGenBuffers_t *)data.data();
      GLuint *result = new GLuint[cmd_data->n];
      glGenBuffers(cmd_data->n, result);

      for (int i = 0; i < cmd_data->n; i++) {
        glGenBuffers_idx_map.insert(std::make_pair(
            cmd_data->last_index - cmd_data->n + 1 + i, result[i]));
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenRenderbuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenRenderbuffers_t *cmd_data =
          (gl_glGenRenderbuffers_t *)data.data();
      GLuint *result = new GLuint[cmd_data->n];
      glGenRenderbuffers(cmd_data->n, result);

      for (int i = 0; i < cmd_data->n; i++) {
        glGenRenderbuffers_idx_map.insert(std::make_pair(
            cmd_data->last_index - cmd_data->n + 1 + i, result[i]));
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindRenderbuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindRenderbuffer_t *cmd_data =
          (gl_glBindRenderbuffer_t *)data.data();
      GLuint buffer_id =
          (GLuint)glGenRenderbuffers_idx_map.find(cmd_data->renderbuffer)
              ->second;

      glBindRenderbuffer(cmd_data->target, buffer_id);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glRenderbufferStorage: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glRenderbufferStorage_t *cmd_data =
          (gl_glRenderbufferStorage_t *)data.data();

      glRenderbufferStorage(cmd_data->target, cmd_data->internalformat,
                            cmd_data->width, cmd_data->height);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glFramebufferRenderbuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glFramebufferRenderbuffer_t *cmd_data =
          (gl_glFramebufferRenderbuffer_t *)data.data();
      glFramebufferRenderbuffer(cmd_data->target, cmd_data->attachment,
                                cmd_data->renderbuffertarget,
                                cmd_data->renderbuffer);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexSubImage3D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexSubImage3D_t *cmd_data = (gl_glTexSubImage3D_t *)data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      if (!buffer_data.empty()) {
        glTexSubImage3D(cmd_data->target, cmd_data->level, cmd_data->xoffset,
                        cmd_data->yoffset, cmd_data->zoffset, cmd_data->width,
                        cmd_data->height, cmd_data->depth, cmd_data->format,
                        cmd_data->type, buffer_data.data());
      } else {
        glTexSubImage3D(cmd_data->target, cmd_data->level, cmd_data->xoffset,
                        cmd_data->yoffset, cmd_data->zoffset, cmd_data->width,
                        cmd_data->height, cmd_data->depth, cmd_data->format,
                        cmd_data->type, NULL);
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindBuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindBuffer_t *cmd_data = (gl_glBindBuffer_t *)data.data();
      GLuint buffer_id =
          (GLuint)glGenBuffers_idx_map.find(cmd_data->id)->second;
      glBindBuffer(cmd_data->target, buffer_id);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindFramebuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindFramebuffer_t *cmd_data =
          (gl_glBindFramebuffer_t *)data.data();
      GLuint buffer_id =
          (GLuint)glGenFramebuffers_idx_map.find(cmd_data->framebuffer)->second;

      glBindFramebuffer(cmd_data->target, buffer_id);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindBufferBase: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindBufferBase_t *cmd_data =
          (gl_glBindBufferBase_t *)data.data();
      GLuint buffer_id =
          (GLuint)glGenBuffers_idx_map.find(cmd_data->buffer)->second;

      glBindBufferBase(cmd_data->target, cmd_data->index, buffer_id);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDeleteTextures: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDeleteTextures_t *cmd_data = (gl_glDeleteTextures_t *)data.data();
     
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
      glDeleteTextures(cmd_data->n, (GLuint*)buffer_data.data());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDeleteFramebuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDeleteFramebuffers_t *cmd_data =
          (gl_glDeleteFramebuffers_t *)data.data();
      
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
      glDeleteFramebuffers(cmd_data->n, (GLuint*) buffer_data.data());
     
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform1f: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform1f_t *cmd_data = (gl_glUniform1f_t *)data.data();
      glUniform1f(cmd_data->location, cmd_data->v0);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glFramebufferTextureLayer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glFramebufferTextureLayer_t *cmd_data =
          (gl_glFramebufferTextureLayer_t *)data.data();
      glFramebufferTextureLayer(cmd_data->target, cmd_data->attachment,
                                cmd_data->texture, cmd_data->level,
                                cmd_data->layer);

      break;
    }
    case (unsigned char)
        GL_Server_Command::GLSC_glRenderbufferStorageMultisample: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glRenderbufferStorageMultisample_t *cmd_data =
          (gl_glRenderbufferStorageMultisample_t *)data.data();
      glRenderbufferStorageMultisample(cmd_data->target, cmd_data->samples,
                                       cmd_data->internalformat,
                                       cmd_data->width, cmd_data->height);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDeleteRenderbuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDeleteRenderbuffers_t *cmd_data =
          (gl_glDeleteRenderbuffers_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      glDeleteRenderbuffers(cmd_data->n, (GLuint*)buffer_data.data());
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glClearBufferfv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glClearBufferfv_t *cmd_data = (gl_glClearBufferfv_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      glClearBufferfv(cmd_data->buffer, cmd_data->drawbuffer, (GLfloat*) buffer_data.data());
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDepthFunc: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDepthFunc_t *cmd_data = (gl_glDepthFunc_t *)data.data();
      glDepthFunc(cmd_data->func);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glColorMask: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glColorMask_t *cmd_data = (gl_glColorMask_t *)data.data();
      glColorMask(cmd_data->red, cmd_data->green, cmd_data->blue,
                  cmd_data->alpha);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glClearDepthf: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glClearDepthf_t *cmd_data = (gl_glClearDepthf_t *)data.data();
      glClearDepth(cmd_data->d);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glFramebufferTexture2D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glFramebufferTexture2D_t *cmd_data =
          (gl_glFramebufferTexture2D_t *)data.data();
      glFramebufferTexture2D(cmd_data->target, cmd_data->attachment,
                             cmd_data->textarget, cmd_data->texture,
                             cmd_data->level);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBufferData: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBufferData_t* cmd_data = (gl_glBufferData_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glBufferData(cmd_data->target, cmd_data->size, buffer_data.data(),
                     cmd_data->usage);
      } else {
        glBufferData(cmd_data->target, cmd_data->size, NULL, cmd_data->usage);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBufferSubData: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBufferSubData_t *cmd_data = (gl_glBufferSubData_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glBufferSubData(cmd_data->target, cmd_data->offset, cmd_data->size,
                        buffer_data.data());
      } else {
        glBufferSubData(cmd_data->target, cmd_data->offset, cmd_data->size,
                        NULL);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform4fv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform4fv_t *cmd_data = (gl_glUniform4fv_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glUniform4fv(cmd_data->location, cmd_data->count, (GLfloat *)buffer_data.data());
      } else {
        glUniform4fv(cmd_data->location, cmd_data->count, NULL);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform1i: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform1i_t *cmd_data = (gl_glUniform1i_t *)data.data();
      glUniform1i(cmd_data->location, cmd_data->v0);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniformBlockBinding: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniformBlockBinding_t *cmd_data =
          (gl_glUniformBlockBinding_t *)data.data();
      glUniformBlockBinding(cmd_data->program, cmd_data->uniformBlockIndex,
                            cmd_data->uniformBlockBinding);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glPixelStorei: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glPixelStorei_t *cmd_data = (gl_glPixelStorei_t *)data.data();
      glPixelStorei(cmd_data->pname, cmd_data->param);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexParameterf: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexParameterf_t *cmd_data = (gl_glTexParameterf_t *)data.data();
      glTexParameterf(cmd_data->target, cmd_data->pname, cmd_data->param);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glVertexAttrib4fv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glVertexAttrib4fv_t *cmd_data = (gl_glVertexAttrib4fv_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glVertexAttrib4fv(cmd_data->index, (GLfloat *)buffer_data.data());
      } else {
        glVertexAttrib4fv(cmd_data->index, NULL);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDrawElements: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDrawElements_t *cmd_data = (gl_glDrawElements_t *)data.data();
      glDrawElements(cmd_data->mode, cmd_data->count, cmd_data->type,
                     (void *)cmd_data->indices);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform2fv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform2fv_t *cmd_data = (gl_glUniform2fv_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glUniform2fv(cmd_data->location, cmd_data->count, (GLfloat *)buffer_data.data());
      } else {
        glUniform2fv(cmd_data->location, cmd_data->count, NULL);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniformMatrix4fv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniformMatrix4fv_t* cmd_data = (gl_glUniformMatrix4fv_t *) data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glUniformMatrix4fv(cmd_data->location, cmd_data->count,
                           cmd_data->transpose, (GLfloat *)buffer_data.data());
      } else {
        glUniformMatrix4fv(cmd_data->location, cmd_data->count,
                           cmd_data->transpose, NULL);
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenVertexArrays: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenVertexArrays_t *cmd_data =
          (gl_glGenVertexArrays_t *)data.data();

      GLuint *result = new GLuint[cmd_data->n];
      glGenVertexArrays(cmd_data->n, result);

      for (int i = 0; i < cmd_data->n; i++) {
        glGenVertexArrays_idx_map.insert(std::make_pair(
            cmd_data->last_index - cmd_data->n + 1 + i, result[i]));
      }

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDrawBuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDrawBuffers_t *cmd_data = (gl_glDrawBuffers_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
      glDrawBuffers(cmd_data->n, (GLenum *)buffer_data.data());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDeleteVertexArrays: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDeleteVertexArrays_t *cmd_data =
          (gl_glDeleteVertexArrays_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
      glDeleteVertexArrays(cmd_data->n, (GLuint*)buffer_data.data());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDeleteBuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDeleteBuffers_t *cmd_data = (gl_glDeleteBuffers_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
      glDeleteBuffers(cmd_data->n, (GLuint*) buffer_data.data());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glReadBuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glReadBuffer_t *cmd_data = (gl_glReadBuffer_t *)data.data();
      glReadBuffer(cmd_data->src);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBlitFramebuffer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBlitFramebuffer_t *cmd_data =
          (gl_glBlitFramebuffer_t *)data.data();
      glBlitFramebuffer(cmd_data->srcX0, cmd_data->srcY0, cmd_data->srcX1,
                        cmd_data->srcY1, cmd_data->dstX0, cmd_data->dstY0,
                        cmd_data->dstX1, cmd_data->dstY1, cmd_data->mask,
                        cmd_data->filter);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindVertexArray: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindVertexArray_t *cmd_data =
          (gl_glBindVertexArray_t *)data.data();
      GLuint array =
          (GLuint)glGenVertexArrays_idx_map.find(cmd_data->array)->second;

      glBindVertexArray(cmd_data->array);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetAttribLocation: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetAttribLocation_t *cmd_data =
          (gl_glGetAttribLocation_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      GLint positionAttr = glGetAttribLocation(cmd_data->programObj,
                                             buffer_data.c_str());

      hasReturn = true;
      ret.rebuild(sizeof(GLint));
      memcpy(ret.data(), &positionAttr, sizeof(GLint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindAttribLocation: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindAttribLocation_t *cmd_data =
          (gl_glBindAttribLocation_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      glBindAttribLocation(cmd_data->program, cmd_data->index,
                           buffer_data.c_str());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glCompressedTexImage2D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glCompressedTexImage2D_t *cmd_data =
          (gl_glCompressedTexImage2D_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      if (!buffer_data.empty()) {
        glCompressedTexImage2D(cmd_data->target, cmd_data->level,
                               cmd_data->internalformat, cmd_data->width,
                               cmd_data->height, cmd_data->border,
                               cmd_data->imageSize, buffer_data.data());
      } else {
        glCompressedTexImage2D(cmd_data->target, cmd_data->level,
                               cmd_data->internalformat, cmd_data->width,
                               cmd_data->height, cmd_data->border,
                               cmd_data->imageSize, NULL);
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetUniformLocation: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetUniformLocation_t *cmd_data =
          (gl_glGetUniformLocation_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      GLint location = glGetUniformLocation(cmd_data->program,
                                            buffer_data.c_str());

      hasReturn = true;
      ret.rebuild(sizeof(GLint));
      memcpy(ret.data(), &location, sizeof(GLint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetUniformBlockIndex: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetUniformBlockIndex_t *cmd_data =
          (gl_glGetUniformBlockIndex_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      GLint index = glGetUniformBlockIndex(cmd_data->program,
                                           buffer_data.c_str());

      hasReturn = true;
      ret.rebuild(sizeof(GLint));
      memcpy(ret.data(), &index, sizeof(GLint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetStringi: {
       // recv data
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetStringi_t *cmd_data = (gl_glGetStringi_t *)data.data();

      const GLubyte *strings = glGetStringi(cmd_data->name, cmd_data->index);

      std::string result;
      result.append(reinterpret_cast<const char *>(strings)); // new style
      
      hasReturn = true;
      ret.rebuild(result.size());
      memcpy(ret.data(), result.data(), result.size());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexSubImage2D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexSubImage2D_t *cmd_data = (gl_glTexSubImage2D_t *)data.data();

      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);


      if (!buffer_data.empty()) {
        glTexSubImage2D(cmd_data->target, cmd_data->level, cmd_data->xoffset,
                        cmd_data->yoffset, cmd_data->width, cmd_data->height,
                        cmd_data->format, cmd_data->type, buffer_data.data());
      } else {
        glTexSubImage2D(cmd_data->target, cmd_data->level, cmd_data->xoffset,
                        cmd_data->yoffset, cmd_data->width, cmd_data->height,
                        cmd_data->format, cmd_data->type, NULL);
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetString: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetString_t *cmd_data = (gl_glGetString_t *)data.data();

      const GLubyte *strings = glGetString(cmd_data->name);

      std::string result;
      result.append(reinterpret_cast<const char *>(strings)); // new style
      hasReturn = true;
      ret.rebuild(result.size());
      memcpy(ret.data(), result.data(), result.size());

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glVertexAttribPointer: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glVertexAttribPointer_t *cmd_data =
          (gl_glVertexAttribPointer_t *)data.data();
      glVertexAttribPointer(cmd_data->index, cmd_data->size, cmd_data->type,
                            cmd_data->normalized, cmd_data->stride,
                            (void *)cmd_data->pointer); // pointer add 0

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glEnableVertexAttribArray: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glEnableVertexAttribArray_t *cmd_data =
          (gl_glEnableVertexAttribArray_t *)data.data();
      glEnableVertexAttribArray(cmd_data->index);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUseProgram: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUseProgram_t *cmd_data = (gl_glUseProgram_t *)data.data();
      glUseProgram(cmd_data->program);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glActiveTexture: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glActiveTexture_t *cmd_data = (gl_glActiveTexture_t *)data.data();
      glActiveTexture(cmd_data->texture);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBindTexture: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBindTexture_t *cmd_data = (gl_glBindTexture_t *)data.data();
      GLuint texture =
          (GLuint)glGenTextures_idx_map.find(cmd_data->texture)->second;

      glBindTexture(cmd_data->target, texture);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glTexImage2D: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glTexImage2D_t *cmd_data = (gl_glTexImage2D_t *)data.data();
     
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);

      if (!buffer_data.empty()) {
        glTexImage2D(cmd_data->target, cmd_data->level,
        cmd_data->internalformat,
                     cmd_data->width, cmd_data->height, cmd_data->border,
                     cmd_data->format, cmd_data->type,
                     (const void *)buffer_data.data());
      } else {
        glTexImage2D(cmd_data->target, cmd_data->level,
        cmd_data->internalformat,
                     cmd_data->width, cmd_data->height, cmd_data->border,
                     cmd_data->format, cmd_data->type, NULL);
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glClearColor: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glClearColor_t *cmd_data = (gl_glClearColor_t *)data.data();
      glClearColor(cmd_data->red, cmd_data->green, cmd_data->blue,
                   cmd_data->alpha);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDrawArrays: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDrawArrays_t *cmd_data = (gl_glDrawArrays_t *)data.data();
      glDrawArrays(cmd_data->mode, cmd_data->first, cmd_data->count);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glViewport: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glViewport_t *cmd_data = (gl_glViewport_t *)data.data();
      glViewport(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height);

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glScissor: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glScissor_t *cmd_data = (gl_glScissor_t *)data.data();
      glScissor(cmd_data->x, cmd_data->y, cmd_data->width, cmd_data->height);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_BREAK: {
      quit = true;
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetIntegerv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetIntegerv_t *cmd_data = (gl_glGetIntegerv_t *)data.data();
      
      GLint result;
      glGetIntegerv(cmd_data->pname, &result);
      
      hasReturn = true;
      ret.rebuild(sizeof(int));
      memcpy(ret.data(), &result, sizeof(GLint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGetFloatv: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGetFloatv_t *cmd_data = (gl_glGetFloatv_t *)data.data();

      GLfloat result;
      glGetFloatv(cmd_data->pname, &result);
      
      hasReturn = true;
      ret.rebuild(sizeof(int));
      memcpy(ret.data(), &result, sizeof(GLint));

      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenTextures: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenTextures_t *cmd_data = (gl_glGenTextures_t *)data.data();

      GLuint *result = new GLuint[cmd_data->n];
      glGenTextures(cmd_data->n, result);

      for (int i = 0; i < cmd_data->n; i++) {
        glGenTextures_idx_map.insert(std::make_pair(
        cmd_data->last_index - cmd_data->n + 1 + i, result[i]));
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glGenFramebuffers: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glGenFramebuffers_t *cmd_data =
          (gl_glGenFramebuffers_t *)data.data();

      GLuint *result = new GLuint[cmd_data->n];
      glGenFramebuffers(cmd_data->n, result);

      for (int i = 0; i < cmd_data->n; i++) {
        glGenFramebuffers_idx_map.insert(std::make_pair(
            cmd_data->last_index - cmd_data->n + 1 + i, result[i]));
      }
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform1ui: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform1ui_t *cmd_data = (gl_glUniform1ui_t *)data.data();
      glUniform1ui(cmd_data->location, cmd_data->v0);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glBeginTransformFeedback: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glBeginTransformFeedback_t *cmd_data =
          (gl_glBeginTransformFeedback_t *)data.data();
      glBeginTransformFeedback(cmd_data->primitiveMode);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glEndTransformFeedback: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      glEndTransformFeedback();
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glVertexAttribDivisor: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glVertexAttribDivisor_t *cmd_data =
          (gl_glVertexAttribDivisor_t *)data.data();
      glVertexAttribDivisor(cmd_data->index, cmd_data->divisor);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glDrawArraysInstanced: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glDrawArraysInstanced_t *cmd_data =
          (gl_glDrawArraysInstanced_t *)data.data();
      glDrawArraysInstanced(cmd_data->mode, cmd_data->first, cmd_data->count,
                            cmd_data->instancecount);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glCullFace: {
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glCullFace_t *cmd_data = (gl_glCullFace_t *)data.data();
      glCullFace(cmd_data->mode);
      break;
    }
    case (unsigned char)GL_Server_Command::GLSC_glUniform1iv:{
      // recv data
      zmq::message_t data_msg;
      auto res = sock.recv(data_msg, zmq::recv_flags::none);
      // check cache
      std::string data = insert_or_check_cache(data_cache, key, data_msg);
      gl_glUniform1iv_t *cmd_data = (gl_glUniform1iv_t *)data.data();
      // more data cache check
      zmq::message_t more_data;
      res = sock.recv(more_data, zmq::recv_flags::none); 
      std::string buffer_data = insert_or_check_cache(more_data_cache, key, more_data);
     
      if (!buffer_data.empty()) {
        glUniform1iv(cmd_data->location, cmd_data->count, (GLint *)buffer_data.data());
      } else {
        glUniform1iv(cmd_data->location, cmd_data->count, NULL);
      }

      break;
    }
    
    case (unsigned char)GL_Server_Command::GLSC_bufferSwap: {
      double currentTime = glfwGetTime();
      numOfFrames++;
      if (currentTime - lastTime >= 1.0) {

        printf("%f ms/frame  %d fps %d bytes\n", 1000.0 / double(numOfFrames),
               numOfFrames, total_data_size);
        numOfFrames = 0;
        total_data_size = 0;
        lastTime = currentTime;
      }
      glfwSwapBuffers(window);
      glfwPollEvents();

      // streaming
      if (enableStreaming) {
        unsigned char *pixel_data =
            (unsigned char *)malloc(WIDTH * HEIGHT * 4); // GL_RGBA
        glReadBuffer(GL_FRONT);
        auto start = std::chrono::steady_clock::now();
        glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixel_data);
        auto end = std::chrono::steady_clock::now();
        std::cout << "glReadPixels: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         end - start)
                         .count()
                  << std::endl;

        try {
          auto start = std::chrono::steady_clock::now();
          mq.get()->send(pixel_data, WIDTH * HEIGHT * 4, 0);
          auto end = std::chrono::steady_clock::now();
          std::cout << "send data to mq: "
                    << std::chrono::duration_cast<std::chrono::microseconds>(
                           end - start)
                           .count()
                    << std::endl;

        } catch (boost::interprocess::interprocess_exception &ex) {
          std::cout << ex.what() << std::endl;
        }
      }
      sequence_number = 0;
      hasReturn = true;
      break;
    }
    default:
      break;
    }
    if (hasReturn)
      sock.send(ret, zmq::send_flags::none);
    sequence_number++;
  }
}
