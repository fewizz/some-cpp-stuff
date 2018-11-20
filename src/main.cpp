﻿#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <iostream>
#include "opengl/gl.hpp"
#include "renderer.hpp"
#include <vector>
#include <initializer_list>
#include "freetype.hpp"
#include <fstream>
#include <filesystem>
#include "text_renderer.hpp"
#include <memory>
#include "glm/mat4x4.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "openal/al.hpp"
#include "wav.hpp"
#include "gui.hpp"

using namespace gl;
using namespace std;
using namespace filesystem;

/*int main0() {
	path p("C:\\Program Files (x86)\\OpenAL 1.1 SDK\\samples\\media\\stereo.wav");
	ifstream stream(p, std::ios::binary);
	wav::wav_audio_data_provider<ifstream> d{stream};
	d.next();
}*/

int main() {
	al::device al_dev = al::open_device();
	al::context al_context = al_dev.create_context();
	al::make_context_current(al_context);

	cout << al::get_error() << "\n";

	al::buffer buff;
	al::source src;

	cout << al::get_error() << "\n";

	path p2("C:\\Program Files (x86)\\OpenAL 1.1 SDK\\samples\\media\\stereo.wav");
	ifstream stream2(p2, std::ios::binary);
	wav::wave_sample_provider<ifstream> d{ stream2 };
	unsigned s_size = 400000;
	char* samples = new char[s_size];
	d.get(samples, s_size);
	//d.next();
	//buff.data(al::buffer::format::mono16, ;;
	buff.data(d.channels(), d.bits_per_sample(), samples, samples + s_size, d.sample_rate());

	cout << al::get_error() << "\n";
	src.buffer(buff);
	src.play();

	cout << al::get_error() << "\n";

	path p("C:\\Windows\\Fonts\\comic.ttf");
	ifstream stream(p, std::ios::binary);
	size_t size = file_size(p);
	char* bytes = new char[size];
	stream.read(bytes, size);
	freetype::face face = freetype::load(bytes, bytes + size);
	//delete bytes;
	face.set_char_size(64*60, 64*60, 0, 0);
	//face.load_glyph(83);

	GLFWwindow* window;
	int w = 800;
	int h = 600;

	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	window = glfwCreateWindow(w, h, "Hello", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	gl::context* glc = wrap_context();
	debug_message_callback([](message_source source, message_type type, unsigned id, message_severity severity, unsigned length,
		const char* message, const void* userParam) {
		std::cout << message << "\n";
	});

	text_renderer tr{ u8"Так хмхмхм\nТы тута ?\nТобе куды песат ? ", face, make_shared<program>(
	vertex_shader{R"(
#version 420 core

uniform mat4 u_mat;

in vec2 a_position;
in vec2 a_uv;
in int a_texture_unit;

out vec2 uv_vs;
out flat int texture_unit_vs;

void main() {
	uv_vs = a_uv;
	texture_unit_vs = a_texture_unit;
	gl_Position = u_mat * vec4(a_position, 0, 1);
}
)"},
	fragment_shader{R"(
#version 420 core

uniform sampler2D u_textures[16];

in vec2 uv_vs;
in flat int texture_unit_vs;
out vec4 color;

void main() {
	color = texture(u_textures[texture_unit_vs], uv_vs);
}

)"}
	) };
	tr.matrix("u_mat", [&w, &h]() {
		return glm::ortho(-(float)w / 2.0f, (float)w / 2.0f, -(float)h / 2.0f, (float)h / 2.0f);
	});

	class main_menu_screen : public gui::square_view {
		class button : public gui::square_view {
			std::string title;
			text_renderer renderer;
		public:
			button(std::string title) :title{title} {

			}

			void render() override {}
			void update() override {}
		} start, quit;

	public:
		void render() override {
			start.render();
		}
		void update() override {}
	} main_menu;

	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &w, &h);
		viewport(0, 0, w, h);
		clear_color(0, 0, 0, 1);
		clear({ color_buffer });

		tr.render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}