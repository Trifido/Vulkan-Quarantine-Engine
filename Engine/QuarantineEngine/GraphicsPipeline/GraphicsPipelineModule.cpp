#include "GraphicsPipelineModule.h"

GraphicsPipelineModule::GraphicsPipelineModule()
{
    keyboard_ptr = KeyboardController::getInstance();
    keyboard_ptr->Attach(this);

    this->gp_fill = new GraphicsPipeline(GraphicsPipeline::PolygonRenderType::FILL);
    this->gp_line = new GraphicsPipeline(GraphicsPipeline::PolygonRenderType::LINE);
    this->gp_point = new GraphicsPipeline(GraphicsPipeline::PolygonRenderType::POINT);

    this->gp_current = this->gp_fill;
}
void GraphicsPipelineModule::Initialize(AntiAliasingModule& AAModule, std::shared_ptr<ShaderModule> SModule, SwapChainModule& SCModule, DepthBufferModule& DBModule, std::shared_ptr<DescriptorModule> DModule)
{
    this->gp_fill->addAntialiasingModule(AAModule);
    this->gp_fill->addShaderModules(SModule);
    this->gp_fill->createRenderPass(SCModule.swapChainImageFormat, DBModule);
    this->gp_fill->createGraphicsPipeline(SCModule.swapChainExtent, DModule->getDescriptorSetLayout());

    this->gp_line->addAntialiasingModule(AAModule);
    this->gp_line->addShaderModules(SModule);
    this->gp_line->createRenderPass(SCModule.swapChainImageFormat, DBModule);
    this->gp_line->createGraphicsPipeline(SCModule.swapChainExtent, DModule->getDescriptorSetLayout());

    this->gp_point->addAntialiasingModule(AAModule);
    this->gp_point->addShaderModules(SModule);
    this->gp_point->createRenderPass(SCModule.swapChainImageFormat, DBModule);
    this->gp_point->createGraphicsPipeline(SCModule.swapChainExtent, DModule->getDescriptorSetLayout());

    shaderModule_ptr = SModule;
}

void GraphicsPipelineModule::cleanup()
{
    shaderModule_ptr->cleanup();
    this->gp_fill->cleanup();
    this->gp_line->cleanup();
    this->gp_point->cleanup();
}

void GraphicsPipelineModule::Update(const __int8& message_from_subject)
{
    updatePolygonMode(message_from_subject);
}

void GraphicsPipelineModule::updatePolygonMode(__int8 polygonType)
{
    switch (polygonType)
    {
    default:
    case 1:
        this->gp_current = this->gp_fill;
        break;
    case 2:
        this->gp_current = this->gp_line;
        break;
    case 3:
        this->gp_current = this->gp_point;
        break;
    }
}
