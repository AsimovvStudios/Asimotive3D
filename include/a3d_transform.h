#pragma once

#include <cglm/cglm.h>

typedef struct {
	mat4     model;
	mat4     view;
	mat4     proj;
} a3d_mvp;

void a3d_mvp_compose(mat4 out, const a3d_mvp* in);
