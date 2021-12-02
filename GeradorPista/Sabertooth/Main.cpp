// Internal
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <math.h>

// External Libs
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <SOIL.h>

// GLM Includes
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

// Headers
#include "Shader.h"
#include "AssetManager.h"
#include "Time.h"
#include "Program.h"

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

// program
Program program;

int main() {
	// window init
	GLFWwindow* window = program.GLFWInit();

	// run
	program.Run(program.coreShader, window);

	// finish
	program.Finish();

	return 0;
}