#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

// TODO fuse Qt and vulkan
#include <QApplication>
#include <QVulkanInstance>
#include <QWindow>

#include <algorithm>
#include <iostream>
#include <array>

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
private:
  QVulkanInstance qVkInstance;
  void exposeEvent(QExposeEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  bool event(QEvent*) override;
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

  // queue to do some work.
  vk::DeviceQueueCreateInfo queueInfo;
  queueInfo.queueFamilyIndex = 0; // Hardcoded this queue family because I know it supports everything I want.
  std::array<float, 1> queuePriorities{1.0f};
  queueInfo.queueCount = 1;
  queueInfo.pQueuePriorities = queuePriorities.data();
  
  //Create a logical device
  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  std::array<const char*, 2> devExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_swapchain"};
  deviceInfo.enabledExtensionCount = devExtensions.size();
  deviceInfo.ppEnabledExtensionNames = devExtensions.data();
  vk::Device device{physDevice.createDevice(deviceInfo)};

  // Setup qt stuff
  QGuiApplication qapp(argc, argv);
  VulkanWindow vkWindow(instance);
  vkWindow.resize(100,100);
  vkWindow.show(); // you have to show the window before you can get it's surface.

  // Fetch surface and info.
  vk::SurfaceKHR surface{QVulkanInstance::surfaceForWindow(&vkWindow)};
  vk::SurfaceCapabilitiesKHR surfaceCap{physDevice.getSurfaceCapabilitiesKHR(surface)};

  // Compute the current extent
  VkExtent2D extent{
    static_cast<uint32_t>(vkWindow.size().width()),
    static_cast<uint32_t>(vkWindow.size().height())
  };
  extent.width = std::max(surfaceCap.minImageExtent.width,
                          std::min(surfaceCap.maxImageExtent.width, extent.width));
  extent.width = std::max(surfaceCap.minImageExtent.height,
                          std::min(surfaceCap.maxImageExtent.height, extent.height));

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

  // Create image views
  std::vector<vk::Image> swapChainImages{device.getSwapchainImagesKHR(swapChain)};
  std::vector<vk::ImageView> swapChainImageViews{swapChainImages.size()};
  std::transform(swapChainImages.begin(), swapChainImages.end(), swapChainImageViews.begin(),
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

  return qapp.exec();
}
