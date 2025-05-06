#include "engine/app/Renderer.hpp"
#include "engine/configs/PipelineConfig.hpp"
#include "engine/pipeline/ShaderModule.hpp"
namespace engine {

Renderer::Renderer(Device &device, PhysicalDevice &physical, Surface &surface,
                   vk::Extent2D windowExtent, Queue::FamilyIndices queues)
    : _device(device), _physical(physical), _surface(surface),
      _extent(windowExtent), _queues(queues),
      _swapchain(physical.get(), device.get(), surface.get(), windowExtent,
                 queues),
      _depth(physical.get(), device.get(), windowExtent),
      _renderPass(device.get(), _swapchain.imageFormat()),
      _pipeline(device.get(),
                /*layout*/ PipelineLayout(device.get(), {/*…*/}),
                _renderPass.get(), vk::PipelineVertexInputStateCreateInfo{},
                vk::PipelineInputAssemblyStateCreateInfo{},
                vk::PipelineRasterizationStateCreateInfo{},
                vk::PipelineMultisampleStateCreateInfo{},
                vk::PipelineDepthStencilStateCreateInfo{},
                vk::PipelineColorBlendStateCreateInfo{}, {/*…*/}, {/*…*/},
                vk::PipelineDynamicStateCreateInfo{},
                GraphicsPipeline::Config{windowExtent}),
      _cmdPool(device.get(), queues.graphics.value()) {
  createSwapchain();
  createDepthBuffer();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPoolAndBuffers();
  createSyncObjects();
  recordCommandBuffers();
}

void Renderer::drawFrame() {}

void Renderer::createFramebuffers() {
  std::vector<ImageView> colorViews;
  auto const &images = _swapchain.images();
  colorViews.reserve(images.size());
  for (auto img : images) {
    colorViews.emplace_back(_device.get(), img, _swapchain.imageFormat(),
                            vk::ImageAspectFlagBits::eColor, 1, 1);
  }

  _framebuffers.clear();
  _framebuffers.reserve(images.size());

  for (size_t i = 0; i < images.size(); ++i) {
    std::array<vk::ImageView, 2> attachments = {
        static_cast<vk::ImageView>(colorViews[i].get()),
        static_cast<vk::ImageView>(_depth.getView())};

    _framebuffers.emplace_back(
        _device.get(), _renderPass.get(), _extent,
        std::vector<vk::ImageView>{attachments.begin(), attachments.end()});
  }
}

void Renderer::createSyncObjects() {
  size_t n = _framebuffers.size();

  _imageAvailable.clear();
  _renderFinished.clear();
  _inFlightFences.clear();

  _imageAvailable.reserve(n);
  _renderFinished.reserve(n);
  _inFlightFences.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    _imageAvailable.emplace_back(_device.get());
    _renderFinished.emplace_back(_device.get());
    _inFlightFences.emplace_back(_device.get(), true);
  }
}

void Renderer::createSwapchain() {
  _swapchain = Swapchain(_physical.get(), _device.get(), _surface.get(),
                         _extent, _queues);
}

void Renderer::createDepthBuffer() {
  _depth = DepthBuffer(_physical.get(), _device.get(), _extent);
}

void Renderer::createRenderPass() {
  _renderPass = RenderPass(_device.get(), _swapchain.imageFormat());
}

void Renderer::createGraphicsPipeline() {
  auto cfg = defaultPipelineConfig(_extent);

  ShaderModule vert{_device.get(), "shaders/triangle.vert.spv"};
  ShaderModule frag{_device.get(), "shaders/triangle.frag.spv"};

  std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
      vert.stageInfo(vk::ShaderStageFlagBits::eVertex),
      frag.stageInfo(vk::ShaderStageFlagBits::eFragment)};

  _pipeline = GraphicsPipeline(
      _device.get(),
      PipelineLayout{_device.get(), cfg.setLayouts, cfg.pushConstants},
      _renderPass.get(), cfg.vertexInput, cfg.inputAssembly, cfg.rasterizer,
      cfg.multisampling, cfg.depthStencil, cfg.colorBlend,
      {stages[0], stages[1]},
      std::vector<vk::PipelineViewportStateCreateInfo>{cfg.viewportState},
      cfg.dynamicState,
      GraphicsPipeline::Config{cfg.viewportExtent, cfg.msaaSamples});
}

void Renderer::recordCommandBuffers() {
  for (size_t i = 0; i < _cmdBuffers.size(); ++i) {
    auto cmd = _cmdBuffers[i].get();
    vk::CommandBufferBeginInfo bi{};
    cmd.begin(bi);

    vk::RenderPassBeginInfo rpbi{};
    rpbi.setRenderPass(*_renderPass.get())
        .setFramebuffer(*_framebuffers[i].get())
        .setRenderArea({{0, 0}, _extent})
        .setClearValueCount(1)
        .setPClearValues(std::array<vk::ClearValue, 1>{
            {vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.3f, 1.0f}}}}
                             .data());

    cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
    // no draw calls yet
    cmd.endRenderPass();
    cmd.end();
  }
}

void Renderer::createCommandPoolAndBuffers() {
  _cmdBuffers.clear();
  _cmdBuffers.reserve(_framebuffers.size());
  for (size_t i = 0; i < _framebuffers.size(); ++i) {
    // this constructor will internally allocate one vk::CommandBuffer
    _cmdBuffers.emplace_back(_device.get(), _cmdPool.get());
  }
}
} // namespace engine
