#include <iostream>
#include <memory>
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "gl.h"
#include "lodepng.h"
#include <string>
#include <vector>
#include <initializer_list>
#include "asset_loader.h"
#include "glm/glm.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "renderer.h"

#define DEF_W 1000
#define DEF_H 600

using namespace gl;
using namespace std;

context* glc;

void debug(initializer_list<const string> lst) {
	for(auto s:lst)cout << s;
	cout << endl;
}
void debug(const string str) { debug({str}); }

int main() {
	if (!glfwInit()) debug("GLFW init error\n");
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(DEF_W, DEF_H, "Test", NULL, NULL);

	glfwMakeContextCurrent(window);

	glc = wrap_context();
	debug("GL context wrapped");
	debug_message_callback([](message_source, message_type, unsigned, message_severity, unsigned, const char* message, const void*) {
		debug({ "GL: ", message });
	});

	glm::mat4 vm{};
	glm::mat4 pm = glm::ortho(-DEF_W / 2.f, DEF_W / 2.f, -DEF_H / 2.f, DEF_H / 2.f, 0.f, 1.f);

	clear_color(0, 0, 0, 1);

	//texture_2d tex = asset_loader::load_texture2d("assets/bimo.png");
	//debug("Bimo loaded to va");

	//std::move
	/*program pr {
		vertex_shader(
			R"(
			#version 430 core
			layout(location = 0) in vec2 pos;
			out vec2 texc;
			void main() {
				texc = pos;
				gl_Position = vec4(pos, 0., 1.);
			}
			)"),
		fragment_shader(
			R"(
			#version 430 core
			out vec4 color;
			in vec2 texc;
			layout(binding = 0) uniform sampler2D tex;
			void main() {
				color = texture(tex, vec2(texc.x, -texc.y));
			}
			)")
	};*/

	/*array_buffer verts;
	verts.data(buffer_usage::static_draw,
		{
		0.F, 1.F,
		1.F, 0.F,
		1.F, 1.F,

		0.F, 0.F,
		1.F, 0.F,
		0.F, 1.F
		});

	vertex_array vao;
	vao.vertex_attrib_pointer<float, 2>(verts, 0);*/

	quad_renderer rend({ 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 });

	while (!glfwWindowShouldClose(window)) {
		clear({color_buffer});
		rend.render(pm);
		//draw_arrays(primitive_type::triangles, 0, 6, pr, vao, { {0, &tex} });
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	return 0;
}