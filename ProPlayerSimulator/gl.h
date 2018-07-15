#ifndef GL_WRAP_H
#define GL_WRAP_H

#include <exception>
#include <string>
#include <initializer_list>
#include <vector>

//#define DEST(cname) ~cname() {if(name)del();}
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
	public:
		unsigned* name{ new unsigned{} };
		virtual void del() {}
		virtual ~with_name() {
			if (name)
				this->del();
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
	class texture2d;

	class context {
	public:
		/*void bind_texture_unit(texture tex, GLuint index) {
			tex.bind();
			glActiveTexture(tex.target);
		}*/
	};

	context* wrap_context();

	enum buffer_target : unsigned {
		array_buffer = 0x8892,
		element_array_buffer = 0x8893,
		pixel_pack_buffer = 0x88EB,
		pixel_unpack_buffer = 0x88EC
	};
	enum buffer_usage : unsigned {
		static_draw = 0x88E4
	};

	class buffer : public gennable, public bindable {
		friend vertex_array;
		friend texture2d;
		static void gl_bind(unsigned target, unsigned name);
	protected:
		const buffer_target target;
		void bind() override { gl_bind(target, *name); };
		void gen() override;
	public:
		buffer(buffer_target tar) :target{ tar } {
			gen();
			bind();
		}

		void del() override;
		void data(size_t bytes, const void * data, buffer_usage usage);

		template<class T>
		void data(std::initializer_list<T> lst, buffer_usage usage) {
			data(sizeof(T)*lst.size(), lst.begin(), usage);
		}
	};

	enum vertex_attrib_pointer_type :unsigned {
	};

	class vertex_array : public gennable, public bindable {
		void gen() override;
		void gl_vertex_attrib_pointer(unsigned index, unsigned size, unsigned type, bool normalized, unsigned stride, void* pointer);
	public:
		void bind() override;
		void del() override;
		vertex_array() {
			gen();
			bind();
		}

		template<class T, int size>
		void vertex_attrib_pointer(buffer& buff, unsigned index, bool normalized = false) {
			bind();
			buff.bind();
			gl_vertex_attrib_pointer(index, size, gl_type_token<T>(), normalized, 0, nullptr);
		}

		void enable_vertex_attrib_array(unsigned index);
	};

	enum internal_format:unsigned {
		rgba8 = 0x8058
	};

	enum pixel_format:unsigned {
		rgba = 0x1908
	};

	enum texture_target :unsigned {
		texture_2d = 0x0DE1
	};

	class texture : public gennable, public bindable {
		void gen() override;
	public:
		void bind() override;
		void del() override;

		texture(texture_target t):target{t} {
			gen();
			bind();
		}
		texture_target target;
	};       

	class texture2d : public texture {
		void gl_tex_sub_image_2d(unsigned target, unsigned level, unsigned xo, unsigned yo, unsigned w, unsigned h, unsigned format, unsigned type, void* p);
	public:
		texture2d():texture(texture_target::texture_2d){}

		template<class T>
		void image(internal_format if_, unsigned w, unsigned h, pixel_format pf, std::vector<T> data);

		template<class T>
		void sub_image(unsigned level, unsigned xoff, unsigned yoff, unsigned w, unsigned h, pixel_format pf, std::vector<T> data) {
			buffer::gl_bind(buffer_target::pixel_unpack_buffer, 0);
			bind();
			gl_tex_sub_image_2d(texture_target::texture_2d, level, xoff, yoff, w, h, pf, gl_type_token<T>(), data.data());
		}

		template<class T>
		void sub_image(buffer& buff, unsigned level, unsigned xoff, unsigned yoff, unsigned w, unsigned h, pixel_format pf) {
			bind();
			buff.bind();
			gl_tex_sub_image_2d(texture_target::texture_2d, level, xoff, yoff, w, h, pf, gl_type_token<T>, nullptr);
		}


		void storage(unsigned levels, internal_format if_, unsigned w, unsigned h);
	};

	class texture_rect :public texture2d {
	public:
		texture_rect() :texture2d() {}
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

	enum shader_type :unsigned {
		compute_shader = 0x91B9,
		vertex_shader = 0x8B31,
		fragment_shader = 0x8B30
	};

	class shader:public creatable {
		friend program;
	protected:
		void create() override;
	public:
		void del() override;
		const shader_type type;
		shader(shader_type type):type{type} {create();}
		shader(shader_type type, std::string src) :shader(type) { source(src); compile(); }

		void source(std::string src);

		void compile();
	};

	class program :public creatable {
	protected:
		void create() override;
	public:
		void del() override;
		program() { create(); }
		program(std::initializer_list<std::pair<shader_type, char*>> init, 
			void(errorh)(shader_compilation_error error) = [](shader_compilation_error err) {})
		{
			create();
			try {
				for (auto p : init) {
					shader s(p.first, p.second);
					attach(s);
				}
			}
			catch (shader_compilation_error e) { errorh(e); }
			link();
		}
		void attach(shader& sh);
		void link();
		void use();
		unsigned attrib_location(std::string attrib_name);
	};

	class framebuffer : public gennable, public bindable {
		framebuffer() {
			gen();
			bind();
		}
	};

	enum primitive_type :unsigned {
		triangles = 0x0004
	};

	void draw_arrays(primitive_type pt, unsigned start, unsigned count, program prog,
		vertex_array vao, std::initializer_list<std::pair<unsigned, texture>> texture_units = {});

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
}


#endif