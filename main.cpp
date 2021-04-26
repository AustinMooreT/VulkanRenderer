#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <algorithm>
#include <iostream>
#include <array>

int main() {

  //Basic instance setup and getting my gpu

  vk::InstanceCreateInfo instanceInfo;
  auto instance = vk::createInstance(instanceInfo);
  auto physDevices = instance.enumeratePhysicalDevices();
  auto physGpu = physDevices[0];

  //


  //Create a queue to do some work.
  
  std::array<float, 1> queuePriorities = {1.0f};
  vk::DeviceQueueCreateInfo queueInfo;
  queueInfo.queueFamilyIndex = 0;
  queueInfo.queueCount = 1;
  queueInfo.setQueuePriorities(queuePriorities);
  
  //
  
  //Create a physical device
  
  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  deviceInfo.setEnabledExtensionCount(1);

  // TODO Finish up device creation. I think I need to enable the swapchain extension.

  return 0;

}
