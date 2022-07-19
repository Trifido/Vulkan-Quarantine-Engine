#include "GraphicsPipeline.h"


GraphicsPipeline::GraphicsPipeline()
{
    this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
}

GraphicsPipeline::GraphicsPipeline(PolygonRenderType type)
{
    switch (type)
    {
    default:
    case PolygonRenderType::FILL:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
        break;
    case PolygonRenderType::LINE:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
        break;
    case PolygonRenderType::POINT:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_POINT;
        break;
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
}

void GraphicsPipeline::createGraphicsPipeline(VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = this->getInputTopology();
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    rasterizer.polygonMode = this->PoligonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    //multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = *antialias_ptr->msaaSamples;
    // multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
    multisampling.minSampleShading = .2f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    /*
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    */

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    // Setup the push constants. It's a single mat4 in the vertex shader.
    VkPushConstantRange pushConstantInfo = { 0 };
    pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantInfo.offset = 0;
    pushConstantInfo.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Number of descriptor sets
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Ptr to descriptor set layout
    pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantInfo; // Optional

    if (vkCreatePipelineLayout(deviceModule->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderModules.at(0)->shaderStages.size());
    pipelineInfo.pStages = shaderModules.at(0)->shaderStages.data();
    pipelineInfo.pVertexInputState = &shaderModules.at(0)->vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional Dynamic state
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(deviceModule->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

void GraphicsPipeline::addShaderModules(std::shared_ptr<ShaderModule> shader_module)
{
    shaderModules.push_back(shader_module);
}

void GraphicsPipeline::cleanup()
{
    vkDestroyPipeline(deviceModule->device, graphicsPipeline, nullptr);
    GraphicsPipelineBase::cleanup();
}

void GraphicsPipeline::updatePolygonMode(PolygonRenderType polygonType)
{
    switch (polygonType)
    {
    default:
    case PolygonRenderType::FILL:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
        break;
    case PolygonRenderType::LINE:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
        break;
    case PolygonRenderType::POINT:
        this->PoligonMode = VkPolygonMode::VK_POLYGON_MODE_POINT;
        break;
    }
}

VkPrimitiveTopology GraphicsPipeline::getInputTopology()
{
    switch (inputTopology)
    {
    default:
    case GraphicsPipelineBase::TRIANGLE_LIST:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case GraphicsPipelineBase::LINES:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case GraphicsPipelineBase::POINTS:
        return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    }
}
