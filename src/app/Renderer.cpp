#include "engine/app/Renderer.hpp"

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
  createFramebuffers();
  createCommandPoolAndBuffers();
  createSyncObjects();
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

} // namespace engine
