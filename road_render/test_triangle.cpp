// glew3, OpenGL ES 3.0, triangle sample
#include <string>
#include <iostream>
#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glt/gles2.hpp"
#include "glt/program.hpp"

using std::string;
using std::cout, std::endl;

using program = glt::shader::program<
	glt::shader::module<glt::shader::gles2_shader_type>>;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

string shader_program_code = R"(
// #version 100
#ifdef _VERTEX_
attribute vec3 position;
void main()	{
	gl_Position = vec4(position,1);
}
#endif
#ifdef _FRAGMENT_
precision mediump float;
void main() {
	gl_FragColor = vec4(1,0,0,1);
}
#endif
)";

constexpr GLfloat vertices[] = {
	0.0f, 0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
};

int main(int argc, char * argv[]) 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	
	GLFWwindow * window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, NULL, NULL);
	assert(window);
	glfwMakeContextCurrent(window);

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	bool err = glewInit() != GLEW_OK;
	
	program flat;
	flat.from_memory(shader_program_code, 100);

	GLint position_attr = flat.attribute_location("position");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glEnableVertexAttribArray(position_attr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);

		flat.use();
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);
	}
	
	glDeleteBuffers(1, &vbo);
	glfwTerminate();
	
	return 0;
}
