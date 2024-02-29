#include "shader.h"

#include "core/types.h"
#include "containers/string.h"
#include "containers/hash_table.h"
#include "math/mats/matrix4.h"
#include "core/logger.h"
#include "fileio/fileio.h"

#include <glad/glad.h>

bool shader_compile_from_file(Shader& shader, const String filepath, Shader::Type type)
{
    String source = file_load_string(filepath);
    bool result = shader_compile_source(shader, source, type);
    free(source);

    return result;
}

bool shader_compile_source(Shader& shader, const String source, Shader::Type type)
{
    // Since I just know fragment and vertex shaders, this works lol
    GLenum gl_shader_type = GL_FRAGMENT_SHADER + (int) type;
    u32 id = glCreateShader(gl_shader_type);

    glShaderSource(id, 1, &(source.data), nullptr);
    glCompileShader(id);

#ifdef GN_DEBUG
    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
    
    if (compile_status != GL_TRUE)
    {
        GLsizei log_length = 0;
        GLchar message[1024];

        glGetShaderInfoLog(id, 1024, &log_length, message);
        print("Shader Error: %\n", ref(message, log_length));

        return false;
    }
#endif // GN_DEBUG

    shader.ids[(int) type] = id;

    return true;
}

bool shader_link(Shader& shader)
{
    shader.program = glCreateProgram();
    glAttachShader(shader.program, shader.ids[0]);
    glAttachShader(shader.program, shader.ids[1]);
    glLinkProgram(shader.program);

#ifdef GN_DEBUG
    GLint link_status;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &link_status);
    
    if (link_status != GL_TRUE)
    {
        GLsizei log_length = 0;
        GLchar message[1024];

        glGetProgramInfoLog(shader.program, 1024, &log_length, message);
        print("Shader Error: %\n", ref(message, log_length));

        return false;
    }
#endif // GN_DEBUG
    
    glDeleteShader(shader.ids[0]);
    glDeleteShader(shader.ids[1]);

    shader.uniforms = make<HashTable<String, s32>>();

    return true;
}

void shader_bind(const Shader& shader)
{
    glUseProgram(shader.program);
}

static inline s32 get_uniform_location(Shader& shader, String uniform_name)
{
    auto elem = find(shader.uniforms, uniform_name);
    if (elem)
        return elem.value();

    s32 uniform_location = glGetUniformLocation(shader.program, uniform_name.data);
    gn_assert_with_message(uniform_location >= 0, "Uniform not found in shader! (name: %)", uniform_name);

    put(shader.uniforms, uniform_name, uniform_location);
    return uniform_location;
}

void shader_set_uniform_1i(Shader& shader, String uniform_name, s32 v0)
{
    glUniform1i(get_uniform_location(shader, uniform_name), v0);
}

void shader_set_uniform_1iv(Shader& shader, String uniform_name, u32 count, s32* vs)
{
    glUniform1iv(get_uniform_location(shader, uniform_name), count, vs);
}

void shader_set_uniform_1f(Shader& shader, String uniform_name, f32 v0)
{
    glUniform1f(get_uniform_location(shader, uniform_name), v0);
}

void shader_set_uniform_1fv(Shader& shader, String uniform_name, u32 count, f32* vs)
{
    glUniform1fv(get_uniform_location(shader, uniform_name), count, vs);
}

void shader_set_uniform_2f(Shader& shader, String uniform_name, f32 v0, f32 v1)
{
    glUniform2f(get_uniform_location(shader, uniform_name), v0, v1);
}

void shader_set_uniform_2fv(Shader& shader, String uniform_name, u32 count, f32* vs)
{
    glUniform2fv(get_uniform_location(shader, uniform_name), count, vs);
}

void shader_set_uniform_3f(Shader& shader, String uniform_name, f32 v0, f32 v1, f32 v2)
{
    glUniform3f(get_uniform_location(shader, uniform_name), v0, v1, v2);
}

void shader_set_uniform_3fv(Shader& shader, String uniform_name, u32 count, f32* vs)
{
    glUniform3fv(get_uniform_location(shader, uniform_name), count, vs);
}

void shader_set_uniform_4f(Shader& shader, String uniform_name, f32 v0, f32 v1, f32 v2, f32 v3)
{
    glUniform4f(get_uniform_location(shader, uniform_name), v0, v1, v2, v3);
}

void shader_set_uniform_4fv(Shader& shader, String uniform_name, u32 count, f32* vs)
{
    glUniform4fv(get_uniform_location(shader, uniform_name), count, vs);
}

void shader_set_uniform_mat4(Shader& shader, String uniform_name, const Matrix4& mat)
{
    glUniformMatrix4fv(get_uniform_location(shader, uniform_name), 1, false, (f32*) mat.data);
}