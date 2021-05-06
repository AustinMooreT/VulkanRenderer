#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan.h>

#include <QApplication>
#include <QVulkanInstance>
#include <QWindow>

#include <range/v3/all.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <array>
#include <fstream>
#include <filesystem>
#include <ranges>

class VulkanWindow : public QWindow {
public:
  VulkanWindow(const vk::Instance& instance) {
    setSurfaceType(QSurface::VulkanSurface);
    qVkInstance.setVkInstance(instance);
    if(qVkInstance.create()) {
      //TODO blow up
    }
    this->setVulkanInstance(&this->qVkInstance);
  }
  void setRenderer(const std::function<void()>& renderer) {
    this->render = renderer;
  }
private:
  QVulkanInstance qVkInstance;
  std::function<void()> render;
  void exposeEvent(QExposeEvent*) override {
    if(this->isExposed()) {
      this->render();
    }
  };
  //void resizeEvent(QResizeEvent*) override;
  //bool event(QEvent*) override;
};

int main(int argc, char** argv) {
  // Basic instance setup and getting my gpu
  vk::InstanceCreateInfo instanceInfo;
  std::array<const char*, 3> instExtensions{VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_surface", "VK_KHR_xcb_surface"};
  instanceInfo.enabledExtensionCount = instExtensions.size();
  instanceInfo.ppEnabledExtensionNames = instExtensions.data();
  vk::Instance instance{vk::createInstance(instanceInfo)};
  std::vector<vk::PhysicalDevice> physDevices{instance.enumeratePhysicalDevices()};
  vk::PhysicalDevice physDevice{physDevices[0]}; // Just hardcoded the index for a gpu that should have vulkan support.
  //
  // Queue setup to do some work.
  vk::DeviceQueueCreateInfo queueInfo;
  queueInfo.queueFamilyIndex = 0; // Hardcoded this queue family because I know it supports everything I want.
  std::array<float, 1> queuePriorities{1.0f};
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = queuePriorities.data();
  //
  //Create a logical device
  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  std::array<const char*, 2> devExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_swapchain"};
  deviceInfo.enabledExtensionCount = devExtensions.size();
  deviceInfo.ppEnabledExtensionNames = devExtensions.data();
  vk::Device device{physDevice.createDevice(deviceInfo)};
  //
  // Setup qt stuff
  QGuiApplication qapp(argc, argv);
  VulkanWindow vkWindow(instance);
  vkWindow.resize(100,100);
  vkWindow.show(); // you have to show the window before you can get it's surface.
  //
  // Fetch surface and info.
  vk::SurfaceKHR surface{QVulkanInstance::surfaceForWindow(&vkWindow)};
  vk::SurfaceCapabilitiesKHR surfaceCap{physDevice.getSurfaceCapabilitiesKHR(surface)};
  //
  // Compute the current extent
  VkExtent2D extent{
    static_cast<uint32_t>(vkWindow.size().width()),
    static_cast<uint32_t>(vkWindow.size().height())
  };
  extent.width = std::max(surfaceCap.minImageExtent.width,
                          std::min(surfaceCap.maxImageExtent.width, extent.width));
  extent.width = std::max(surfaceCap.minImageExtent.height,
                          std::min(surfaceCap.maxImageExtent.height, extent.height));
  //
  // Create SwapChain
  vk::SwapchainCreateInfoKHR swapChainInfo;
  swapChainInfo.surface = surface;
  swapChainInfo.minImageCount = surfaceCap.minImageCount + 1;
  swapChainInfo.imageFormat = vk::Format::eB8G8R8A8Srgb; // blindly assume image format
  swapChainInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear; // blindly assume color space
  swapChainInfo.presentMode = vk::PresentModeKHR::eMailbox; // blindly assume present mode
  swapChainInfo.imageArrayLayers = 1; // Only not one when developing stereoscopic 3D app.
  swapChainInfo.imageExtent = extent;
  swapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  swapChainInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainInfo.preTransform = surfaceCap.currentTransform;
  swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainInfo.presentMode = vk::PresentModeKHR::eMailbox; // blindly assuming mailbox presentation mode
  swapChainInfo.clipped = VK_TRUE;
  vk::SwapchainKHR swapChain{device.createSwapchainKHR(swapChainInfo)};
  //
  // Create image views
  std::vector<vk::Image> swapChainImages{device.getSwapchainImagesKHR(swapChain)};
  std::vector<vk::ImageView> swapChainImageViews{swapChainImages.size()};
  std::ranges::transform(swapChainImages, swapChainImageViews.begin(),
                         [&device](const vk::Image& image) -> vk::ImageView {
                           vk::ImageViewCreateInfo imageInfo;
                           imageInfo.image = image;
                           imageInfo.viewType = vk::ImageViewType::e2D;
                           imageInfo.format = vk::Format::eB8G8R8A8Srgb; // Blindly assume image format
                           imageInfo.components.r = vk::ComponentSwizzle::eIdentity;
                           imageInfo.components.g = vk::ComponentSwizzle::eIdentity;
                           imageInfo.components.b = vk::ComponentSwizzle::eIdentity;
                           imageInfo.components.a = vk::ComponentSwizzle::eIdentity;
                           imageInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                           imageInfo.subresourceRange.baseMipLevel = 0;
                           imageInfo.subresourceRange.levelCount = 1;
                           imageInfo.subresourceRange.baseArrayLayer = 0;
                           imageInfo.subresourceRange.layerCount = 1;
                           return device.createImageView(imageInfo);
                         });
  //
  // Render pass setup
  // Basic subpass setup
  // Color buffer attachment
  vk::AttachmentDescription colorAttachmentDescription;
  colorAttachmentDescription.format = vk::Format::eB8G8R8A8Srgb;
  colorAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
  colorAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachmentDescription.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  vk::AttachmentReference colorAttachmentReference;
  colorAttachmentReference.attachment = 0;
  colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;  
  //
  // Subpass creation
  vk::SubpassDescription basicSubpass;
  basicSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  basicSubpass.colorAttachmentCount = 1;
  basicSubpass.pColorAttachments = &colorAttachmentReference;
  //
  //
  // Render pass creation
  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachmentDescription;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &basicSubpass;
  vk::SubpassDependency subpassDependency;
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass = 0;
  subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  subpassDependency.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
  subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &subpassDependency;
  vk::RenderPass renderPass = device.createRenderPass(renderPassInfo);
  //
  //
  // Fixed Pipeline Setup
  // Setup vertex input
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  //
  // Setup input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssemblyInfo.primitiveRestartEnable = false;
  //
  // Setup viewport
  vk::Viewport viewport;
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vk::Rect2D scissor;
  scissor.offset = vk::Offset2D{0, 0};
  scissor.extent = extent;
  vk::PipelineViewportStateCreateInfo viewPortStateInfo;
  viewPortStateInfo.viewportCount = 1;
  viewPortStateInfo.pViewports = &viewport;
  viewPortStateInfo.scissorCount = 1;
  viewPortStateInfo.pScissors = &scissor;
  //
  // Setup rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
  rasterizerInfo.depthClampEnable = false;
  rasterizerInfo.rasterizerDiscardEnable = false;
  rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
  rasterizerInfo.lineWidth = 1.0f;
  rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack;
  rasterizerInfo.frontFace = vk::FrontFace::eClockwise;
  rasterizerInfo.depthBiasEnable = false;
  //
  // Setup multisample
  vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
  multisamplingInfo.sampleShadingEnable = false;
  multisamplingInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
  //
  // depth and/or stencil buffer config would happen at this stage as well, but skip it for now.
  //
  // Setup color blending for framebuffers.
  vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
  colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR |
    vk::ColorComponentFlagBits::eB | 
    vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eA;
  colorBlendAttachmentState.blendEnable = false;
  vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo{};
  colorBlendStateInfo.logicOpEnable = false;
  colorBlendStateInfo.attachmentCount = 1;
  colorBlendStateInfo.pAttachments = &colorBlendAttachmentState;
  //
  // dynamic pipeline state can be specified here.
  //
  // Pipeline layout
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
  vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
  //
  // End Fixed function pipeline setup
  // Setup shader pipeline
  std::function<std::vector<uint32_t>(const std::filesystem::path&)> getShader = 
    [](const std::filesystem::path& filepath) {
      std::vector<uint32_t> fileData;
      std::ifstream shaderFile(filepath, std::ios::ate | std::ios::binary);
      if(!shaderFile.is_open()) {
        return fileData;
      } else {
        std::size_t fileSize{static_cast<std::size_t>(shaderFile.tellg())};
        shaderFile.seekg(0);
        fileData.reserve(fileSize);
        shaderFile.read(reinterpret_cast<char*>(fileData.data()), fileSize);
        return fileData;
      }
    };
  auto triangleVertData{getShader("shaders/triangle.vert.spv")};
  auto triangleFragData{getShader("shaders/triangle.frag.spv")};
  vk::ShaderModuleCreateInfo triangleVertModuleInfo;
  vk::ShaderModuleCreateInfo triangleFragModuleInfo;
  triangleVertModuleInfo.setCode(triangleVertData);
  triangleFragModuleInfo.setCode(triangleFragData);
  std::cout << "vert\n";
  vk::ShaderModule triangleVertShader{device.createShaderModule(triangleVertModuleInfo)};
  std::cout << "bet\n";
  vk::ShaderModule triangleFragShader{device.createShaderModule(triangleFragModuleInfo)};
  std::cout << "frag\n";
  vk::PipelineShaderStageCreateInfo shaderTriangleVertPipelineInfo;
  vk::PipelineShaderStageCreateInfo shaderTriangleFragPipelineInfo;
  shaderTriangleVertPipelineInfo.stage = vk::ShaderStageFlagBits::eVertex;
  shaderTriangleFragPipelineInfo.stage = vk::ShaderStageFlagBits::eFragment;
  shaderTriangleVertPipelineInfo.pName = "main";
  shaderTriangleFragPipelineInfo.pName = "main";
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderPipelineInfo{shaderTriangleVertPipelineInfo,
    shaderTriangleFragPipelineInfo};
  //
  // Actually instantiate the pipeline
  vk::GraphicsPipelineCreateInfo graphicsPipelineInfo;
  graphicsPipelineInfo.stageCount = shaderPipelineInfo.size();
  graphicsPipelineInfo.pStages = shaderPipelineInfo.data();
  graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
  graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  graphicsPipelineInfo.pViewportState = &viewPortStateInfo;
  graphicsPipelineInfo.pRasterizationState = &rasterizerInfo;
  graphicsPipelineInfo.pMultisampleState = &multisamplingInfo;
  graphicsPipelineInfo.pColorBlendState = &colorBlendStateInfo;
  graphicsPipelineInfo.layout = pipelineLayout;
  graphicsPipelineInfo.renderPass = renderPass;
  graphicsPipelineInfo.subpass = 0;
  vk::Pipeline graphicsPipeline{device.createGraphicsPipeline(VK_NULL_HANDLE, graphicsPipelineInfo).value};
  // Create framebuffers
  std::vector<vk::Framebuffer> frameBuffers{swapChainImageViews.size()};
  std::ranges::transform(swapChainImageViews, frameBuffers.begin(), 
                         [&renderPass, &extent, &device](const vk::ImageView& imageView) {
                           std::array<vk::ImageView, 1> imageViewAttachment{imageView};
                           vk::FramebufferCreateInfo frameBufferInfo;
                           frameBufferInfo.renderPass = renderPass;
                           frameBufferInfo.attachmentCount = 1;
                           frameBufferInfo.pAttachments = imageViewAttachment.data();
                           frameBufferInfo.width = extent.width;
                           frameBufferInfo.height = extent.height;
                           frameBufferInfo.layers = 1;
                           return device.createFramebuffer(frameBufferInfo);
                         });
  //Setup command buffers.
  vk::CommandPoolCreateInfo commandPoolInfo;
  commandPoolInfo.queueFamilyIndex = 0;
  vk::CommandPool commandPool{device.createCommandPool(commandPoolInfo)};
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
  commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(frameBuffers.size());
  std::vector<vk::CommandBuffer> commandBuffers{device.allocateCommandBuffers(commandBufferAllocateInfo)};
  //
  //
  for(auto&& [frameBuffer, commandBuffer] : ranges::views::zip(frameBuffers, commandBuffers)) {
    // Setup recording into command buffer
    vk::CommandBufferBeginInfo commandBufferBeginInfo;
    vk::RenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffer;
    renderPassBeginInfo.renderArea.offset = vk::Offset2D{0,0};
    renderPassBeginInfo.renderArea.extent = extent;
    vk::ClearValue clearColor{std::array{0.0f, 0.0f, 0.0f, 1.0f}}; // black
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;
    //
    // Begin recording
    commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRenderPass();
    //
  }
  //
  
  vk::SemaphoreCreateInfo semaphoreInfo;
  vk::Semaphore imageIsAvailable{device.createSemaphore(semaphoreInfo)};
  vk::Semaphore renderingFinished{device.createSemaphore(semaphoreInfo)};

  auto drawFrame = [&device, &swapChain, &imageIsAvailable, &commandBuffers, &renderingFinished, &graphicsPipeline]() {
    uint32_t imageIndex = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageIsAvailable, VK_NULL_HANDLE).value;
    vk::SubmitInfo submitInfo;
    std::array<vk::PipelineStageFlags, 1> waitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageIsAvailable;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderingFinished;
    vk::Queue queue = device.getQueue(0, 0);
    auto idk = queue.submit(1, &submitInfo, VK_NULL_HANDLE); // not sure what this returns
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderingFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    idk = queue.presentKHR(presentInfo); // Pretty sure this returns the same type as submit
  };

  vkWindow.setRenderer(drawFrame);

  return qapp.exec();
}
