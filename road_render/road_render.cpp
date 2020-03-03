// glew3, OpenGL ES 3.0, triangle sample
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <cassert>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include "glt/opengl.hpp"
#include "glt/gles2.hpp"
#include "glt/program.hpp"

using namespace std::chrono_literals;
using std::string,
	std::vector,
	std::cout,
	std::endl;
using glm::vec2;

using program = glt::shader::program<
	glt::shader::module<glt::shader::gles2_shader_type>>;

constexpr GLuint WIDTH = 600,
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

// 4 nodes
constexpr float sample_road[] = {
	0, -0.5f,
	0, 0.3f,
	0.3f, 0.6f,
	0.8f, 0.6f
};

GLuint push_road(float const * road_nodes, size_t node_count, float road_width);
void draw_road(GLuint position_vbo, GLint position_loc, size_t node_count);

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

	bool err = glewInit();
	assert(err == GLEW_OK);
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	program flat;
	flat.from_memory(shader_program_code, 100);

	GLint position_loc = flat.attribute_location("position");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	float road_width = 0.05f;
	GLuint road_vbo = push_road(sample_road, 4, road_width);

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();

		// draw gui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Road");  // begin window

		// ...
		if (ImGui::SliderFloat("width", &road_width, 0.01f, 0.2f))
		{
			glDeleteBuffers(1, &road_vbo);
			road_vbo = push_road(sample_road, 4, road_width);
		}

		ImGui::End();  // end window

		ImGui::Render();


		glClear(GL_COLOR_BUFFER_BIT);

		flat.use();
		draw_road(road_vbo, position_loc, 4);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &road_vbo);
	glfwTerminate();
	
	return 0;
}

void draw_road(GLuint position_vbo, GLint position_loc, size_t node_count)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, node_count * 2);
}

GLuint push_road(float const * road_nodes, size_t node_count, float road_width)
{
	vector<vec2> verts;
	verts.reserve(2*node_count);

	float w = road_width/2;
	for (size_t i = 0; i < node_count; ++i)
	{
		vec2 cur = vec2{road_nodes[2*i], road_nodes[2*i+1]};
		vec2 ds;

		if (i == 0)  // first node
		{
			vec2 next = vec2{road_nodes[2], road_nodes[3]};
			ds = glm::normalize(next - cur);
		}
		else if (i == node_count-1)  // last node
		{
			vec2 prev = vec2{road_nodes[2*(i-1)], road_nodes[2*(i-1)+1]};
			ds = glm::normalize(cur - prev);
		}
		else // middle nodes
		{
			vec2 prev = vec2{road_nodes[2*(i-1)], road_nodes[2*(i-1)+1]},
				next = vec2{road_nodes[2*(i+1)], road_nodes[2*(i+1)+1]};

			ds = glm::normalize(cur - prev);
			ds += glm::normalize(next - cur);
			float l = (ds.x*ds.x + ds.y*ds.y) * 0.5f;
			ds /= l;
		}

		ds *= w;
		verts.push_back(vec2{cur.x - ds.y, cur.y + ds.x});
		verts.push_back(vec2{cur.x + ds.y, cur.y - ds.x});
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*2*sizeof(float), &verts[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return vbo;
}
