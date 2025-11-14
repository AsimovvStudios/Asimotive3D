#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "a3d_logging.h"
#include "a3d_mesh.h"
#include "vulkan/a3d_vulkan_pipeline.h"

#define A3D_SHADER_VERTEX_PATH "shaders/triangle.vert.spv"
#define A3D_SHADER_FRAGMENT_PATH "shaders/triangle.frag.spv"

static bool read_file_binary(const char* path, unsigned char** data, size_t* size);
static VkShaderModule create_shader_module(a3d* engine, const unsigned char* data, size_t size);

bool a3d_vk_create_graphics_pipeline(a3d* engine)
{
	A3D_LOG_INFO("creating graphics pipeline");

	unsigned char* vertex_data = NULL;
	unsigned char* fragment_data = NULL;
	size_t vertex_size = 0;
	size_t fragment_size = 0;

	if (!read_file_binary(A3D_SHADER_VERTEX_PATH, &vertex_data, &vertex_size))
		return false;

	if (!read_file_binary(A3D_SHADER_FRAGMENT_PATH, &fragment_data, &fragment_size)) {
		free(vertex_data);
		return false;
	}

	VkShaderModule vertex_module = create_shader_module(engine, vertex_data, vertex_size);
	VkShaderModule fragment_module = create_shader_module(engine, fragment_data, fragment_size);

	free(vertex_data);
	free(fragment_data);

	if (!vertex_module || !fragment_module) {
		if (vertex_module)
			vkDestroyShaderModule(engine->vk.logical, vertex_module, NULL);
		if (fragment_module)
			vkDestroyShaderModule(engine->vk.logical, fragment_module, NULL);
		return false;
	}

	/* shader stages */
	VkPipelineShaderStageCreateInfo vertex_stage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertex_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragment_stage = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragment_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo stages[] = {vertex_stage, fragment_stage};

	/*
	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
	};
	*/

	VkVertexInputBindingDescription binding = {
		.binding = 0,
		.stride = sizeof(a3d_vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};

	VkVertexInputAttributeDescription attributes[2] = {
		{
			.binding = 0,
			.location = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(a3d_vertex, position)
		},
		{
			.binding = 0,
			.location = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(a3d_vertex, colour)
		}
	};

	VkPipelineVertexInputStateCreateInfo vertex_input = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = attributes,
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	/* viewport & scissor */
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = engine->vk.swapchain_extent.width,
		.height = engine->vk.swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = engine->vk.swapchain_extent
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	/* rasterizer */
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

	/* multisampling (disabled : TODO) */
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

	/* color blend, one color attachment, no blending, write all channels */
	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo color_blend = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment
	};

	/* pipeline layout, no descriptor sets, no push constants : TODO */
	VkPipelineLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

	VkResult result = vkCreatePipelineLayout(engine->vk.logical, &layout_info, NULL, &engine->vk.pipeline_layout);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreatePipelineLayout failed with code %d", result);
		vkDestroyShaderModule(engine->vk.logical, vertex_module, NULL);
		vkDestroyShaderModule(engine->vk.logical, fragment_module, NULL);
		return false;
	}

	/* graphics pipeline */
	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = stages,
		.pVertexInputState = &vertex_input,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = NULL,
		.pColorBlendState = &color_blend,
		.pDynamicState = NULL,
		.layout = engine->vk.pipeline_layout,
		.renderPass = engine->vk.render_pass,
		.subpass = 0
	};

	result = vkCreateGraphicsPipelines(
		engine->vk.logical, VK_NULL_HANDLE, 1,
		&pipeline_info,NULL, &engine->vk.pipeline
	);

	/* shader modules not needed after pipeline baked */
	vkDestroyShaderModule(engine->vk.logical, vertex_module, NULL);
	vkDestroyShaderModule(engine->vk.logical, fragment_module, NULL);

	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateGraphicsPipelines failed with code %d", result);
		vkDestroyPipelineLayout(engine->vk.logical, engine->vk.pipeline_layout, NULL);
		engine->vk.pipeline_layout = VK_NULL_HANDLE;
		return false;
	}

	A3D_LOG_INFO("graphics pipeline created");
	return true;
}

void a3d_vk_destroy_graphics_pipeline(a3d* engine)
{
	if (engine->vk.pipeline) {
		vkDestroyPipeline(engine->vk.logical, engine->vk.pipeline, NULL);
		engine->vk.pipeline = VK_NULL_HANDLE;
		A3D_LOG_INFO("destroyed graphics pipeline");
	}

	if (engine->vk.pipeline_layout) {
		vkDestroyPipelineLayout(engine->vk.logical, engine->vk.pipeline_layout, NULL);
		engine->vk.pipeline_layout = VK_NULL_HANDLE;
		A3D_LOG_INFO("destroyed graphics pipeline layout");
	}
}

static bool read_file_binary(const char* path, unsigned char** data, size_t* size)
{
	FILE* file = fopen(path, "rb");
	if (!file) {
		A3D_LOG_ERROR("failed to open file %s", path);
		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		A3D_LOG_ERROR("fseek failed for file %s", path);
		return false;
	}

	long len = ftell(file);
	if (len < 0) {
		fclose(file);
		A3D_LOG_ERROR("ftell failed for file %s", path);
		return false;
	}

	rewind(file);

	unsigned char* buffer = malloc(len);
	if (!buffer) {
		fclose(file);
		A3D_LOG_ERROR("out of memory reading file %s", path);
		return false;
	}

	size_t read = fread(buffer, 1, len, file);
	fclose(file);

	if (read != (size_t)len) {
		free(buffer);
		A3D_LOG_ERROR("short read for file %s", path);
		return false;
	}

	*data = buffer;
	*size = len;

	return true;
}

static VkShaderModule create_shader_module(a3d* engine, const unsigned char* data, size_t size)
{
	VkShaderModuleCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (const Uint32*)data
	};

	VkShaderModule module = VK_NULL_HANDLE;
	VkResult result = vkCreateShaderModule(engine->vk.logical, &info, NULL, &module);
	if (result != VK_SUCCESS) {
		A3D_LOG_ERROR("vkCreateShaderModule failed with code %d", result);
		return VK_NULL_HANDLE;
	}

	return module;
}
