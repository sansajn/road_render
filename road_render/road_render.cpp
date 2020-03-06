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
attribute vec2 position;
uniform vec3 tile_offset;  // (x: x offset, y: y offset, z: 1/area_width)
void main()	{
	gl_Position = vec4((position - tile_offset.xy) * tile_offset.z, 0, 1);
}
#endif
#ifdef _FRAGMENT_
precision mediump float;
uniform vec4 color;
void main() {
	gl_FragColor = color;
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

vector<GLuint> push_road_network(vector<road> const & roads, float road_width);
vector<vec2> road_vertices(road const & r, float road_width);
GLuint push_road(float const * road_nodes, size_t node_count, float road_width);
GLuint push_road(vector<vec2> const & road_nodes, float road_width);
void draw_road(GLuint position_vbo, GLint position_loc, size_t node_count);
void gui_init(GLFWwindow * window);
GLFWwindow * sample_window();
void dump_road(road const & r, vec3 const & tile_offset);


int main(int argc, char * argv[]) 
{
	string map_file = argc > 1 ? argv[1] : MAP_FILE;

	GLFWwindow * window = sample_window();

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	gui_init(window);

	box roads_area;
	vector<road> roads;
	read_roads(map_file, roads, roads_area);
	assert(!roads.empty());

	float map_w = geom::width(roads_area),
		map_h = geom::height(roads_area),
		area_width = max(map_w, map_h);

	// we want fit to (-1,-1),(1,1) screen
	float scale = 1/area_width;
	vec3 tile_offset = vec3{roads_area.min_corner() + vec2{
		map_w/2.f, map_h/2.f}, scale*2};

	cout << "there were " << size(roads) << " roads found in '" << map_file
		<< "' map file\n"
		<< "in a " << dsv(roads_area) << " area\n"
		<< "area width=" << map_w << ", height=" << map_h << "\n"
		<< "min area corner is " << dsv(roads_area.min_corner()) << "\n"
		<< "tile offset is " << dsv(tile_offset) << "\n";

	// road 1
	cout << "road_0:\n";
	dump_road(roads[0], tile_offset);

	program flat;
	flat.from_memory(shader_program_code, 100);
	GLint position_loc = flat.attribute_location("position");
	program::uniform_type tile_offset_u = flat.uniform_variable("tile_offset"),
		color_u = flat.uniform_variable("color");

	vec4 road_color = vec4{rgb::white, 1};
	float road_width = 0.05f;
	GLuint road_vbo = push_road(sample_road, 4, road_width);

	vector<GLuint> road_vbos = push_road_network(roads, road_width);

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
		if (ImGui::SliderFloat("width", &road_width, 0.01f, 0.2f))
		{
			glDeleteBuffers(1, &road_vbo);
			road_vbo = push_road(sample_road, 4, road_width);

			glDeleteBuffers(size(road_vbos), &road_vbos[0]);
			road_vbos = push_road_network(roads, road_width);
		}

		ImGui::End();  // end window

		ImGui::Render();


		glClear(GL_COLOR_BUFFER_BIT);

		flat.use();
		tile_offset_u = vec3{0,0,1};
		color_u = road_color;
		draw_road(road_vbo, position_loc, 4);

		tile_offset_u = tile_offset;
		for (size_t i = 0; i < 1/*size(road_vbos)*/; ++i)
			draw_road(road_vbos[i], position_loc, size(roads[i].geometry));

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

vector<GLuint> push_road_network(vector<road> const & roads, float road_width)
{
	size_t road_count = size(roads);

	vector<GLuint> vbos;
	vbos.resize(road_count);

	glGenBuffers(road_count, &vbos[0]);

	for (size_t i = 0; i < road_count; ++i)
	{
		// TODO: vertices buffer can be reused
		vector<vec2> verts = road_vertices(roads[i], road_width);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, verts.size()*2*sizeof(float), &verts[0], GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return vbos;
}

vector<vec2> road_vertices(road const & r, float road_width)
{
	size_t node_count = size(r.geometry);

	vector<vec2> verts;
	verts.reserve(2*node_count);

	vector<vec2> const & road_nodes = r.geometry;

	float w = road_width/2;
	for (size_t i = 0; i < node_count; ++i)
	{
		vec2 const & cur = road_nodes[i];
		vec2 ds;

		if (i == 0)  // first node
		{
			vec2 const & next = road_nodes[i+1];
			ds = glm::normalize(next - cur);
		}
		else if (i == node_count-1)  // last node
		{
			vec2 const & prev = road_nodes[i-1];
			ds = glm::normalize(cur - prev);
		}
		else // middle nodes
		{
			vec2 const & prev = road_nodes[i-1],
			next = road_nodes[i+1];

			ds = glm::normalize(cur - prev);
			ds += glm::normalize(next - cur);
			float l = (ds.x*ds.x + ds.y*ds.y) * 0.5f;
			ds /= l;
		}

		ds *= w;
		verts.push_back(vec2{cur.x - ds.y, cur.y + ds.x});
		verts.push_back(vec2{cur.x + ds.y, cur.y - ds.x});
	}

	return verts;
}

GLuint push_road(vector<vec2> const & road_nodes, float road_width)
{
	vector<vec2> verts;
	verts.reserve(2*size(road_nodes));

	float w = road_width/2;
    for (size_t i = 0; i < size(road_nodes); ++i)
	{
		vec2 const & cur = road_nodes[i];
		vec2 ds;

		if (i == 0)  // first node
		{
			vec2 const & next = road_nodes[i+1];
			ds = glm::normalize(next - cur);
		}
		else if (i == size(road_nodes)-1)  // last node
		{
			vec2 const & prev = road_nodes[i-1];
			ds = glm::normalize(cur - prev);
		}
		else // middle nodes
		{
			vec2 const & prev = road_nodes[i-1],
				next = road_nodes[i+1];

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

void dump_road(road const & r, vec3 const & tile_offset)
{
	cout << "nodes=" << size(r.geometry) << "\n";
	for (vec2 const & n : r.geometry)
	{
		vec2 n_ = (n - vec2{tile_offset.x, tile_offset.y}) * tile_offset.z;
		cout << dsv(n_) << ", ";
	}
	cout << "\n";
}
