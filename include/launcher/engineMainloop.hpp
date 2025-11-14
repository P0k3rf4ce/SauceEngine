#ifndef ENGINE_MAINLOOP_HPP
#define ENGINE_MAINLOOP_HPP

#include "launcher/optionParser.hpp"

struct GLFWwindow;

void initGLFW();
GLFWwindow* initWindow(unsigned int scr_width, unsigned int scr_height);
bool initGLAD();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
int engine_mainloop(const AppOptions &ops);

// Precision should be up to a millisecond
inline double get_seconds_since_epoch();

#endif