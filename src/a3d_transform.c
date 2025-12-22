#include <cglm/cglm.h>

#include "a3d_transform.h"

void a3d_mvp_compose(mat4 out, const a3d_mvp* in)
{
	mat4 pv;
	/* cglm api doesnt use const for input mats. cast away const here */
	glm_mat4_mul((vec4*)in->proj, (vec4*)in->view, pv);
	glm_mat4_mul(pv, (vec4*)in->model, out);
}