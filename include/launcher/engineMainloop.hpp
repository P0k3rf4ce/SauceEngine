#ifndef ENGINE_MAINLOOP_HPP
#define ENGINE_MAINLOOP_HPP

#include "launcher/optionParser.hpp"
#include "utils/Camera.hpp"

struct GLFWwindow;

void initGLFW();
GLFWwindow* initWindow(unsigned int scr_width, unsigned int scr_height);
bool initGLAD();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, double deltatime, std::shared_ptr<Camera> camera);
int engine_mainloop(const AppOptions &ops);

// Precision should be up to a millisecond
inline double get_seconds_since_epoch();

#endif
