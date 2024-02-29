#pragma once

#include "core/types.h"
#include "containers/string.h"
#include "containers/hash_table.h"
#include "math/mats/matrix4.h"

struct Shader
{
    enum struct Type
    {
        FRAGMENT,
        VERTEX,
        NUM_TYPES
    };

    u32 ids[(u32) Type::NUM_TYPES];
    u32 program;
    HashTable<String, s32> uniforms;
};

bool shader_compile_from_file(Shader& shader, const String filepath, Shader::Type type);
bool shader_compile_source(Shader& shader, const String source, Shader::Type type);
bool shader_link(Shader& shader);

void shader_bind(const Shader& shader);

void shader_set_uniform_1i(Shader& shader, String uniform_name, s32 v0);
void shader_set_uniform_1iv(Shader& shader, String uniform_name, u32 count, s32* vs);

void shader_set_uniform_1f(Shader& shader, String uniform_name, f32 v0);
void shader_set_uniform_1fv(Shader& shader, String uniform_name, u32 count, f32* vs);

void shader_set_uniform_2f(Shader& shader, String uniform_name, f32 v0, f32 v1);
void shader_set_uniform_2fv(Shader& shader, String uniform_name, u32 count, f32* vs);

void shader_set_uniform_3f(Shader& shader, String uniform_name, f32 v0, f32 v1, f32 v2);
void shader_set_uniform_3fv(Shader& shader, String uniform_name, u32 count, f32* vs);

void shader_set_uniform_4f(Shader& shader, String uniform_name, f32 v0, f32 v1, f32 v2, f32 v3);
void shader_set_uniform_4fv(Shader& shader, String uniform_name, u32 count, f32* vs);

void shader_set_uniform_mat4(Shader& shader, String uniform_name, const Matrix4& mat);