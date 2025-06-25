#ifndef PSO_HPP
#define PSO_HPP

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <utility>
#include <array>
#include <cstdint>

#include "Shader.h"
#include "IShaderProvider.h"
#include "HlslTypes.h"


struct BufferBindingDesc
{
	uint32_t binding;
	VkDescriptorType type;        // UNIFORM_BUFFER or STORAGE_BUFFER
	VkShaderStageFlags stageFlags;
	size_t size;                  // Size of the buffer (for allocation)
	//std::string name;             // Optional: for debugging/reflection
};
struct RenderPassDesc
{
	std::vector<VkAttachmentDescription> attachments;
	//std::vector<VkAttachmentReference> colorAttachments;
	std::optional<VkAttachmentDescription> depthAttachment;
	//uint32_t subpass = 0;
};
struct PipelineDesc
{
	VkPipelineInputAssemblyStateCreateInfo iaState;
	VkPipelineRasterizationStateCreateInfo rasterState;
	VkPipelineMultisampleStateCreateInfo msState;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
	std::optional<VkPipelineDepthStencilStateCreateInfo> depthStencilState;
};
struct PipelineLayoutDesc
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::vector<VkPushConstantRange> pushConstants;
};
struct DescriptorSetLayoutDesc
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorBindingFlags> bindingFlags;
};

struct PsoDesc
{
public:
	PsoDesc() = delete;
	PsoDesc(const std::string& name, IShaderProvider& provider) :
		shaderProvider{&provider} {}
	std::string name;
	std::vector<BufferBindingDesc> bufferBindingDescs;
	DescriptorSetLayoutDesc descriptorSetLayoutDesc;
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::string vs;
	std::string ps;
	std::string cs;
	RenderPassDesc renderPassDesc;
	PipelineDesc pipelineDesc;
	PipelineLayoutDesc layout;
	IShaderProvider* const shaderProvider;

	Shader* const getVS() {
		if (!vs.empty() || vs != "") {
			return shaderProvider->getShader(vs);
		}
		return nullptr;
	}
	Shader* const getPS() {
		if (!ps.empty() || ps != "") {
			return shaderProvider->getShader(ps);
		}
		return nullptr;
	}
	Shader* const getCS() {
		if (!cs.empty() || cs != "") {
			return shaderProvider->getShader(cs);
		}
		return nullptr;
	}




	// Templates
	static PsoDesc createBase2DPipeline(IShaderProvider& provider) {
		PsoDesc pso{"Basic2D", provider};

		// RENDERPASS
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		pso.renderPassDesc.attachments.push_back(colorAttachment);
		pso.renderPassDesc.attachments.push_back(depthAttachment);

		// PIPELINE
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = 8;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[1].offset = 4;
	
		pso.vertexBindings.push_back(bindingDescription);
		pso.vertexAttributes.push_back(attributeDescriptions[0]);
		pso.vertexAttributes.push_back(attributeDescriptions[1]);


		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
		rasterizer.lineWidth = 1.0f;

		pso.pipelineDesc.rasterState = rasterizer;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		pso.pipelineDesc.msState = multisampling;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		pso.pipelineDesc.colorBlendAttachmentState = colorBlendAttachment;


		std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[0].descriptorCount = 1024;
		bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[2].binding = 0;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings[3].binding = 0;
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[3].descriptorCount = 1;
		bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[3].pImmutableSamplers = nullptr;

		std::array<VkDescriptorBindingFlags, 4> bindingFlags = {
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
				| VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT, // for t0[]
			0, // for t1
			0,  // for s0
			0 // b0
		};
		
		for (size_t i = 0; i < bindings.size(); ++i) {
			pso.descriptorSetLayoutDesc.bindings.push_back(bindings[i]);
			pso.descriptorSetLayoutDesc.bindingFlags.push_back(bindingFlags[i]);
		}

		BufferBindingDesc globalsBuffDesc{};
		globalsBuffDesc.binding = 0;
		globalsBuffDesc.size = sizeof(GlobalData);
		globalsBuffDesc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		globalsBuffDesc.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pso.bufferBindingDescs.push_back(globalsBuffDesc);

		BufferBindingDesc instanceBuffDesc{};
		instanceBuffDesc.binding = 1;
		instanceBuffDesc.size = sizeof(SpriteInstance) * MAX_SPRITE_INSTANCES;
		instanceBuffDesc.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instanceBuffDesc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pso.bufferBindingDescs.push_back(instanceBuffDesc);
	}
};

#endif