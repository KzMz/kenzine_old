#pragma once

#include "math_defines.h"

void geometry_generate_normals(u32 vertex_count, Vertex3d* vertices, u32 index_count, u32* indices);
void geometry_generate_tangents(u32 vertex_count, Vertex3d* vertices, u32 index_count, u32* indices);