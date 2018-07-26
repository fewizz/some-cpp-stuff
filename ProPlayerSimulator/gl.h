#pragma once

#include <exception>
#include <string>
#include <initializer_list>
#include <vector>
#include <iostream>
#include <map>
#include <array>

namespace gl {
	class with_name {
	protected:
		template<class T>
		static unsigned constexpr gl_type_token() {
			if (std::is_same_v<T, int8_t>)
				return 0x1400;
			if (std::is_same_v<T, uint8_t>)
				return 0x1401;
			if (std::is_same_v<T, int32_t>)
				return 0x1404;
			if (std::is_same_v<T, uint32_t>)
				return 0x1405;
			if (std::is_same_v<T, float_t>)
				return 0x1406;
			else throw std::exception("GL: can't find appr. type");
		}

		static constexpr unsigned invalid_name = std::numeric_limits<unsigned>::max();

		void invalidate_name() {
			name = invalid_name;
		}
		virtual void del() {}
	public:
		with_name() {};

		with_name(with_name&& o) :name{ o.name } { 
			o.invalidate_name();
		}

		with_name& operator=(with_name&& o) {
			name = o.name;
			o.invalidate_name();
			return *this;
		}

		unsigned name{ invalid_name };
		virtual ~with_name() {
			if (name)
				del();
		}
	};
	class bindable:virtual public with_name {
	protected:
		virtual void bind() = 0;
	};
	class gennable:virtual public with_name {
	protected:
		virtual void gen() = 0;
	};
	class creatable:virtual public with_name {
	protected:
		virtual void create() = 0;
	};
	class vertex_array;
	class program;
	class shader;
	class texture;
	class texture_2d;

	class context {
		//std::map<texture::texture_target, unsigned> tex_targets;
	public:
	};

	context* wrap_context();
	//

	// �4.1
	class fence_sync;
	enum class sync_status:unsigned;

	class fence_sync {
		friend sync_status client_wait_sync(fence_sync, unsigned);
		friend void wait_sync(fence_sync);
		void* ptr;
	public:
		fence_sync();
	};

	enum class sync_status:unsigned {
		already_signaled = 0x911A,
		timeout_expired = 0x911B,
		condition_satisfied = 0x911C,
		wait_failed = 0x911D
	};

	sync_status client_wait_sync(fence_sync, unsigned);
	void wait_sync(fence_sync);

	// �4.2
	// TODO

	class query : public gennable {
		void gen() override;
	};

	enum buffer_usage : unsigned {
		static_draw = 0x88E4
	};

	class buffer : public gennable, public bindable {
		friend vertex_array;
		friend texture_2d;
	protected:
		enum buffer_target : unsigned {
			array = 0x8892,
			element_array = 0x8893,
			pixel_pack = 0x88EB,
			pixel_unpack = 0x88EC
		};

		buffer_target target;
		static unsigned gl_gen();
		static void gl_bind(unsigned target, unsigned name);

		void bind() override { gl_bind(target, name); }
		void gen() override { name = gl_gen(); }
	public:
		buffer(buffer_target tar) : target{tar} {
			gen();
			bind();
		}

		void del() override;
		void data(size_t bytes, const void * data, buffer_usage usage = buffer_usage::static_draw);

		template<class T>
		void data(std::initializer_list<T> lst, buffer_usage usage = buffer_usage::static_draw) {
			data(sizeof(T)*lst.size(), lst.begin(), usage);
		}
	};

	//typedef buffer<buffer_target::array> array_buffer;
	class array_buffer : public buffer { public: array_buffer() :buffer(buffer_target::array) {} };

	enum primitive_type :unsigned;
	void draw_arrays(primitive_type pt, unsigned start, unsigned count, program& prog);

	class vertex_array : public virtual gennable, public virtual bindable {
		friend void draw_arrays(primitive_type, unsigned, unsigned, program&);
		void gen() override;
		void gl_vertex_attrib_pointer(unsigned index, unsigned size, unsigned type, bool normalized, unsigned stride, void* pointer);
		void static gl_bind(unsigned name);
	public:
		void bind() override { gl_bind(name); };
		void del() override;

		vertex_array() {
			gen();
			bind();
		}

		template<class T>
		vertex_array(std::initializer_list<std::pair<unsigned, T*>> bindings) {
			vertex_array();
			bind();
			for (auto p : bindings) {
				array_buffer vbo{buffer_target::array_buffer};
				vbo.data(gl::buffer_usage::static_draw, p.second);
				vertex_attrib_pointer(vbo, p.first);
				enable_vertex_attrib_array(p.first);
			}
		}

		template<class T, int size>
		void vertex_attrib_pointer(unsigned index, buffer& buff, bool normalized = false) {
			bind();
			buff.bind();
			gl_vertex_attrib_pointer(index, size, gl_type_token<T>(), normalized, 0, nullptr);
		}

		template<class T, int size>
		void vertex_attrib_pointer(unsigned index, void* data, size_t bytes, bool normalized = false) {
			array_buffer vbo;
			vbo.data(bytes, data);
			vertex_attrib_pointer<T, size>(index, vbo, normalized);
		}

		void enable_vertex_attrib_array(unsigned index);
	};

	enum internal_format:unsigned {
		rgba8 = 0x8058
	};

	enum pixel_format:unsigned {
		rgba = 0x1908
	};

	class texture : public gennable, public bindable {
		void gen() override;
		friend context;
	protected:
		enum texture_target :unsigned {
			texture_2d = 0x0DE1
		};

		virtual texture_target target() = 0;
	public:
		void bind() override;
		void del() override;

		texture() {
			gen();
			bind();
		}
	};       

	class texture_2d : public texture {
		void gl_tex_sub_image_2d(unsigned target, unsigned level, unsigned xo, unsigned yo, unsigned w, unsigned h, unsigned format, unsigned type, void* p);
	protected:
		texture_target target() override { return texture_target::texture_2d; }
	public:
		template<class T>
		void image(internal_format if_, unsigned w, unsigned h, pixel_format pf, T* data);

		template<class T>
		void sub_image(unsigned level, unsigned xoff, unsigned yoff, unsigned w, unsigned h, pixel_format pf, T* data) {
			buffer::gl_bind(buffer::buffer_target::pixel_unpack, 0);
			bind();
			gl_tex_sub_image_2d(target(), level, xoff, yoff, w, h, pf, gl_type_token<T>(), data);
		}

		template<class T>
		void sub_image(buffer& buff, unsigned level, unsigned xoff, unsigned yoff, unsigned w, unsigned h, pixel_format pf) {
			bind();
			buff.bind();
			gl_tex_sub_image_2d(target(), level, xoff, yoff, w, h, pf, gl_type_token<T>, nullptr);
		}


		void storage(unsigned levels, internal_format if_, unsigned w, unsigned h);
	};

	class texture_rect :public texture_2d {
	public:
		
	};

	/*enum sampler_parameter :GLenum {
		texture_wrap_s
	};*/
	/*enum tex_coord:GLuint {
		s = 1,
		t = 1 << 1,
		p = 1 << 2,
		q = 1 << 3
	};*/
	enum wrap_mode :unsigned {
		clamp_to_border = 0x812D,
		clamp_to_edge = 0x812F,
		repeat = 0x2901
	};

	class sampler :public gennable {
		void gen() override;

	public:
		void del() override;
		void texture_wrap_s(wrap_mode wm);
		void texture_wrap_t(wrap_mode wm);
		void texture_wrap_r(wrap_mode wm);
	};

	class shader_compilation_error : public std::exception {
		friend shader;

		shader_compilation_error(shader*const s, std::string message) :shader_obj{ s }, std::exception(message.c_str()) {}
	public:
		shader * const shader_obj;
	};

	class shader:public creatable {
		friend program;
	protected:
		void create() override;
		enum shader_type :unsigned {
			compute_shader = 0x91B9,
			vertex_shader = 0x8B31,
			fragment_shader = 0x8B30
		};
	public:
		void del() override;
		const shader_type type;
		shader(shader_type type):type{type} {create();}
		shader(shader_type type, std::string src) :shader(type) { source(src); compile(); }

		void source(std::string src);
		void compile();
	};
	class vertex_shader : public shader { public: vertex_shader(std::string src) :shader(shader_type::vertex_shader, src) {} };
	class fragment_shader : public shader { public: fragment_shader(std::string src) :shader(shader_type::fragment_shader, src) {} };

	class program :public creatable {
	protected:
		void create() override;
	public:
		void del() override;
		program() { create(); }
		program(std::initializer_list<shader> init,
			void(errorh)(shader_compilation_error error) = [](shader_compilation_error err) {})
		{
			create();
			try {
				for (auto &s : init)
					attach(s);
			}
			catch (shader_compilation_error e) { errorh(e); }
			link();
		}
		void attach(const shader& sh);
		void link();
		void use();
		unsigned attrib_location(std::string attrib_name);
		void uniform_matrix4fv(unsigned location, unsigned count, bool tr, float* values);
	};

	class framebuffer : public gennable, public bindable {
		framebuffer() {
			gen();
			bind();
		}
	};

	enum primitive_type :unsigned {
		triangles = 0x0004,
		triangles_fan = 0x0006
	};
	void draw_arrays(primitive_type pt, unsigned start, unsigned count, program& prog);
	void draw_arrays(primitive_type pt, unsigned start, unsigned count, program& prog,
		vertex_array& vao, std::initializer_list<std::pair<unsigned, texture*>> texture_units = {});

	enum message_source :unsigned {
	};
	enum message_type :unsigned {
	};
	enum message_severity :unsigned {
	};

	void debug_message_callback(
		void(f)(message_source source, message_type type, unsigned id, message_severity severity, unsigned length,
			const char* message, const void* userParam));

	enum clear_buffer: unsigned {
		color_buffer = 0x00004000,
		depth_buffer = 0x00000100
	};
	void clear_color(float r, float g, float b, float a);
	void clear(std::initializer_list<clear_buffer> mask);
	void vertex_attrib2fv(unsigned index, const float* values);
	void vertex_attrib4fv(unsigned index, const float* values);
}