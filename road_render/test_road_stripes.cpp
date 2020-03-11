// road network rendering using fill shader program
#include <algorithm>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iterator>
#include <iostream>
#include <cassert>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include "glt/opengl.hpp"
#include "glt/gles2.hpp"
#include "glt/program.hpp"
#include "glt/colors.hpp"
#include "road.hpp"
#include "geometry/box2.hpp"

using namespace std::chrono_literals;
using std::string,
	std::vector,
	std::size,
	std::max,
	std::cout,
	std::endl;
using glm::vec2,
	glm::vec3,
	glm::vec4;
using boost::geometry::dsv;

using program = glt::shader::program<
	glt::shader::module<glt::shader::gles2_shader_type>>;

constexpr GLuint WIDTH = 600,
	HEIGHT = 600;

constexpr char MAP_FILE[] = "../maps/highways.osm";

string shader_program_code = R"(
// #version 100

#ifdef _VERTEX_
attribute vec2 position;  // mesh vertices
attribute vec2 uv;  // texture coordinates mapping
uniform vec3 tile_offset;  // (x: x offset, y: y offset, z: 1/area_width)
uniform vec3 stripe_size;
varying vec2 st;

void main()	{
	gl_Position = vec4((position - tile_offset.xy) * tile_offset.z, 0, 1);
	st = uv;
}
#endif

#ifdef _FRAGMENT_
precision mediump float;
uniform vec3 color;
varying vec2 st;

void main() {
	gl_FragColor = vec4(color * st.x, 1);  // st.x is from 0 to 1 in our sample
}
#endif
)";

// 2 nodes
constexpr float sample_road[] = {
	0, -0.75f,
	0, 0.75f
};
constexpr size_t sample_road_node_count = 2;


GLuint push_road(float const * road_nodes, size_t node_count, float road_width);
void draw_road(GLuint road_vbo, GLint position_loc, GLint uv_loc, size_t node_count);
void gui_init(GLFWwindow * window);
GLFWwindow * sample_window();


int main(int argc, char * argv[]) 
{
	GLFWwindow * window = sample_window();

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	gui_init(window);

	vec3 tile_offset = vec3{0, 0, 1};

	program flat;
	flat.from_memory(shader_program_code, 100);
	GLint position_loc = flat.attribute_location("position"),
		uv_loc = flat.attribute_location("uv");
	program::uniform_type tile_offset_u = flat.uniform_variable("tile_offset"),
		color_u = flat.uniform_variable("color");

	cout << "attribute locations:\n"
		<< "postition=" << position_loc << "\n"
		<< "uv=" << uv_loc << "\n";

	assert(position_loc != -1 && uv_loc != -1 && "attribute not active!");

	vec3 road_color = rgb::white;

	float road_width = 0.05f;
	GLuint road_vbo = push_road(sample_road, sample_road_node_count, road_width);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	while (!glfwWindowShouldClose(window)) 
	{
		glfwPollEvents();

		// draw gui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Road");  // begin window

		// ...
		if (ImGui::SliderFloat("width", &road_width, 0.01f, 0.1f))
		{
			glDeleteBuffers(1, &road_vbo);
			road_vbo = push_road(sample_road, sample_road_node_count, road_width);
		}

		ImGui::End();  // end window

		ImGui::Render();


		glClear(GL_COLOR_BUFFER_BIT);

		flat.use();
		color_u = road_color;

		tile_offset_u = tile_offset;
		draw_road(road_vbo, position_loc, uv_loc, sample_road_node_count);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);

		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &road_vbo);
	glfwTerminate();
	
	return 0;
}


void draw_road(GLuint road_vbo, GLint position_loc, GLint uv_loc, size_t node_count)
{
	glEnableVertexAttribArray(position_loc);
	glEnableVertexAttribArray(uv_loc);
	glBindBuffer(GL_ARRAY_BUFFER, road_vbo);
	glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (GLvoid*)0);
	glVertexAttribPointer(uv_loc, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (GLvoid*)(2*sizeof(float)));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, node_count * 2);

	assert(glGetError() == GL_NO_ERROR && "opengl error");
}

GLuint push_road(float const * road_nodes, size_t node_count, float road_width)
{
	vector<vec4> verts;
	verts.reserve(2*node_count);

	float length = 0;
	float w = road_width/2;
	for (size_t i = 0; i < node_count; ++i)
	{
		vec2 cur = vec2{road_nodes[2*i], road_nodes[2*i+1]};
		vec2 ds;

		if (i == 0)  // first node
		{
			vec2 next = vec2{road_nodes[2], road_nodes[3]};
			ds = glm::normalize(next - cur);
			length = 0;
		}
		else if (i == node_count-1)  // last node
		{
			vec2 prev = vec2{road_nodes[2*(i-1)], road_nodes[2*(i-1)+1]};
			ds = glm::normalize(cur - prev);

			length += glm::length(cur - prev);
		}
		else // middle nodes
		{
			vec2 prev = vec2{road_nodes[2*(i-1)], road_nodes[2*(i-1)+1]},
				next = vec2{road_nodes[2*(i+1)], road_nodes[2*(i+1)+1]};

			ds = glm::normalize(cur - prev);
			ds += glm::normalize(next - cur);
			float l = (ds.x*ds.x + ds.y*ds.y) * 0.5f;
			ds /= l;

			length += glm::length(cur - prev);
		}

		ds *= w;
		verts.push_back(vec4{vec2{cur.x - ds.y, cur.y + ds.x}, length, -w});
		verts.push_back(vec4{vec2{cur.x + ds.y, cur.y - ds.x}, length, w});
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size()*4*sizeof(float), &verts[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	assert(glGetError() == GL_NO_ERROR && "opengl error");

	return vbo;
}

void gui_init(GLFWwindow * window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
}

GLFWwindow * sample_window()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow * window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, nullptr, nullptr);
	assert(window);
	glfwMakeContextCurrent(window);

	bool err = glewInit();
	assert(err == GLEW_OK);

	return window;
}

