#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

// TODO fuse Qt and vulkan
#include <QApplication>
#include <QVulkanInstance>
#include <QWindow>

#include <algorithm>
#include <iostream>
#include <array>



int main(int argc, char** argv) {

  // Basic instance setup and getting my gpu
  vk::InstanceCreateInfo instanceInfo;
  std::array<const char*, 2> instExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_xcb_surface"};
  instanceInfo.enabledExtensionCount = 2;
  instanceInfo.ppEnabledExtensionNames = instExtensions.data();
  auto instance = vk::createInstance(instanceInfo);
  auto physDevices = instance.enumeratePhysicalDevices();
  auto physGpu = physDevices[0]; // I just hardcoded the index for my amdgpu

  // queue to do some work.
  std::array<float, 2> queuePriorities = {1.0f, 1.0f};
  vk::DeviceQueueCreateInfo queueInfo;
  queueInfo.queueFamilyIndex = 0; // I hardcode this queue family because I know it supports everything I want.
  queueInfo.queueCount = 2; // I'm going to use 2 queues one for present the other for rendering.
  queueInfo.setQueuePriorities(queuePriorities);
  
  //Create a logical device
  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.enabledExtensionCount = 1;
  std::array<const char*, 1> devExtensions = {"VK_KHR_swapchain"};
  deviceInfo.ppEnabledExtensionNames = devExtensions.data();
  auto device = physGpu.createDevice(deviceInfo);

  //Setup swapchain
  vk::SurfaceKHR surface;



  vk::SwapchainCreateInfoKHR swapInfo;
  swapInfo.surface = surface;
  //  swapInfo.imageFormat = nullptr;
  //  swapInfo.imageColorSpace = nullptr;
  //  swapInfo.imageExtent = nullptr;
  swapInfo.imageArrayLayers = 1;
  swapInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst; 


  //Setup Qt window to handle swapchain and presentation
  class VulkanWindow : public QWindow {
  public:
    VulkanWindow(vk::Instance vkinst) {
      setSurfaceType(QSurface::VulkanSurface);
      this->vkInstance.setVkInstance(vkinst);
      setVulkanInstance(&this->vkInstance);
    }
    virtual ~VulkanWindow();
  private:
    QVulkanInstance vkInstance;
    void exposeEvent(QExposeEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    bool event(QEvent*) override;
  };

  
    
  return 0;

}
