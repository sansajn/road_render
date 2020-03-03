// glew3, OpenGL ES 3.0, triangle sample
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include "glt/opengl.hpp"
#include "glt/gles2.hpp"
#include "glt/program.hpp"

using std::string,
	std::vector,
	std::cout,
	std::endl;
using glm::vec2;

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

// two nodes
constexpr float sample_road[] = {
	0, -0.5f,
	0, 0.5f
};

GLuint push_road(float * road_nodes, size_t node_count, float road_width);

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


GLuint push_road(float * road_nodes, size_t node_count, float road_width)
{
	assert(node_count == 2);

	vec2 n0 = vec2{road_nodes[0], road_nodes[1]},
		n1 = vec2{road_nodes[2], road_nodes[3]};

	vector<vec2> verts(2*node_count);

	float w = road_width/2;
	vec2 d = w * glm::normalize(n1 - n0);
	verts[0] = vec2{n0.x - d.y, n0.y - d.x};
	verts[1] = vec2{n0.x + d.y, n0.y + d.x};

	d = glm::normalize(n0 - n1);
	verts[2] = vec2{n1.x - d.y, n1.y - d.x};
	verts[3] = vec2{n1.x + d.y, n1.y + d.x};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*2*sizeof(float), &verts[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return vbo;
}
