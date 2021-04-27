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
  std::array<const char*, 3> instExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_surface", "VK_KHR_xcb_surface"};
  instanceInfo.enabledExtensionCount = instExtensions.size();
  instanceInfo.ppEnabledExtensionNames = instExtensions.data();
  auto instance = vk::createInstance(instanceInfo);
  auto physDevices = instance.enumeratePhysicalDevices();
  auto physDevice = physDevices[0]; // Just hardcoded the index for a gpu that should have vulkan support.

  // queue to do some work.
  vk::DeviceQueueCreateInfo queueInfo;
  queueInfo.queueFamilyIndex = 0; // Hardcoded this queue family because I know it supports everything I want.
  std::array<float, 2> queuePriorities = {1.0f, 1.0f};
  queueInfo.queueCount = 2; // Going to use 2 queues one for present the other for rendering.
  queueInfo.pQueuePriorities = queuePriorities.data();
  
  //Create a logical device
  vk::DeviceCreateInfo deviceInfo;
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queueInfo;
  std::array<const char*, 2> devExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_swapchain"};
  deviceInfo.enabledExtensionCount = devExtensions.size();
  deviceInfo.ppEnabledExtensionNames = devExtensions.data();
  auto device = physDevice.createDevice(deviceInfo);

  // Configure surface info
  //vk::SurfaceKHR surface;
  // Finds the index of the surface format we want.
  /*
  
  swapInfo.surface = surface;
  // We're just blindly assuming the gpu supports this format and color space.
  swapInfo.imageFormat = vk::Format::eB8G8R8A8Unorm; 
  swapInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  swapInfo.minImageCount = surfaceCap.maxImageCount + 1;
  //  swapInfo.imageExtent = nullptr;
  swapInfo.imageArrayLayers = 1;
  swapInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst; 
  */

  //Setup Qt window to handle swapchain and presentation
  class VulkanWindow : public QWindow {
  public:
    VulkanWindow(const vk::Instance& vkinst, const vk::PhysicalDevice& vkdev) {
      setSurfaceType(QSurface::VulkanSurface);
      this->qVkInstance.setVkInstance(vkinst);
      this->setVulkanInstance(&this->qVkInstance);
      if(!this->qVkInstance.create()) {
        //TODO blow up
      }
      this->vkPhysDev = vkdev;
    }
    //virtual ~VulkanWindow();
  private:
    vk::SurfaceKHR vkSurface;
    vk::PhysicalDevice vkPhysDev;
    QVulkanInstance qVkInstance;
    //void exposeEvent(QExposeEvent*) override;
    //void resizeEvent(QResizeEvent*) override;
    //bool event(QEvent*) override;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCap) {
      VkExtent2D currentExtent{
        static_cast<uint32_t>(this->size().width()),
        static_cast<uint32_t>(this->size().height())
      };
      currentExtent.width = std::max(surfaceCap.minImageExtent.width,
                                     std::min(surfaceCap.maxImageExtent.width, currentExtent.width));
      currentExtent.width = std::max(surfaceCap.minImageExtent.height,
                                     std::min(surfaceCap.maxImageExtent.height, currentExtent.height));
      return currentExtent;
    }

    void createSwapChain() {
      if(this->size().isEmpty()) { // sanity check.
		return;
      }
	
	QVulkanInstance *inst = vulkanInstance();
	vkDeviceWaitIdle(device_);
	
	//==> New
	SwapchainSupportDetails details = QuerySwapchainSupport(phys_device_);
	
	VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(details.formats);
	VkPresentModeKHR present_mode = ChooseSwapPresentMode(details.present_modes);
	VkExtent2D image_extent = ChooseSwapExtent(details.capabilities);
	
	uint32_t image_count = details.capabilities.minImageCount + 1;
	const auto max = details.capabilities.maxImageCount;
	
	/* Note:
		If max = 0 means there is no limit besides
		memory requirements, which is why we need to check for that.
	*/
	if (max > 0 && image_count > max)
		image_count = max;
	//<== New
	
	VkSurfaceTransformFlagBitsKHR pre_transform = 
	(details.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
	: details.capabilities.currentTransform;
	
	VkCompositeAlphaFlagBitsKHR composite_alpha =
	(details.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	? VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	
	VkSwapchainKHR oldSwapChain = swapchain_;
	
	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface_;
	swapchain_info.minImageCount = image_count;//reqBufferCount;
	swapchain_info.imageFormat = surface_format.format;//color_format_;
	swapchain_info.imageColorSpace = surface_format.colorSpace;//colorSpace;
	swapchain_info.imageExtent = image_extent; //bufferSize;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.preTransform = pre_transform;
	swapchain_info.compositeAlpha = composite_alpha;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = true;
	swapchain_info.oldSwapchain = oldSwapChain;
	
//	mtl_line("Creating new swapchain of %d buffers, size %dx%d", image_count,
//		image_extent.width, image_extent.height);
	
	VkSwapchainKHR newSwapChain;
	
	VkResult result = vkCreateSwapchainKHR_(device_, &swapchain_info, nullptr,
	&newSwapChain);
	
	if (result != VK_SUCCESS)
		qFatal("Failed to create swap chain: %d", result);
	
	if (oldSwapChain)
		ReleaseSwapchain();
	
	swapchain_ = newSwapChain;
	
	result = vkGetSwapchainImagesKHR_(device_, swapchain_,
		&swapchain_buffer_count_, nullptr);

	if (result != VK_SUCCESS || swapchain_buffer_count_ < 2)
	{
		qFatal("Failed to get swapchain images: %d (count=%d)", result,
			swapchain_buffer_count_);
	}
		
//	mtl_line("Swapchain buffer count: %d", swapchain_buffer_count_);

	image_resources_.resize(swapchain_buffer_count_);
	VkImage swapchain_images[swapchain_buffer_count_];
	
	result = vkGetSwapchainImagesKHR_(device_, swapchain_,
		&swapchain_buffer_count_, swapchain_images);
	
	if (result != VK_SUCCESS)
		qFatal("Failed to get swapchain images: %d", result);
	
	// Now that we know color_format_, create the default renderpass, the
	// framebuffers will need it.
	CreateDefaultRenderPass();
	
	VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr,
	VK_FENCE_CREATE_SIGNALED_BIT};
	
	for (uint32_t i = 0; i < swapchain_buffer_count_; ++i) {
		ImageResources &image(image_resources_[i]);
		image.image = swapchain_images[i];
		
		VkImageViewCreateInfo imgViewInfo;
		memset(&imgViewInfo, 0, sizeof(imgViewInfo));
		imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgViewInfo.image = swapchain_images[i];
		imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgViewInfo.format = color_format_;
		imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgViewInfo.subresourceRange.levelCount =
		imgViewInfo.subresourceRange.layerCount = 1;
		result = vkCreateImageView(device_, &imgViewInfo, nullptr,
		&image.image_view);
		if (result != VK_SUCCESS)
			qFatal("Failed to create swapchain image view %d: %d", i, result);
		
		result = vkCreateFence(device_, &fenceInfo, nullptr, &image.cmd_fence);
		
		if (result != VK_SUCCESS)
			qFatal("Failed to create command buffer fence: %d", result);
		
		image.cmd_fence_waitable = true;
		
		VkImageView views[1] = {image.image_view};
		VkFramebufferCreateInfo fbInfo = {};
		fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass = default_render_pass_;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = views;
		fbInfo.width = swapchain_image_size_.width();
		fbInfo.height = swapchain_image_size_.height();
		fbInfo.layers = 1;
		result = vkCreateFramebuffer(device_, &fbInfo, nullptr, &image.fb);
		
		if (result != VK_SUCCESS)
			qFatal("Failed to create framebuffer: %d", result);
	}
	
	current_image_ = 0;
	
	VkSemaphoreCreateInfo semInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	nullptr, 0};
	for (uint32_t i = 0; i < FRAME_LAG; ++i) {
		FrameResources &frame(frame_resources_[i]);
		vkCreateFence(device_, &fenceInfo, nullptr, &frame.fence);
		frame.fenceWaitable = true;
		vkCreateSemaphore(device_, &semInfo, nullptr, &frame.imageSem);
		vkCreateSemaphore(device_, &semInfo, nullptr, &frame.drawSem);
	}
	
	current_frame_ = 0;
    }
  };


  QGuiApplication qapp(argc, argv);
  VulkanWindow vw(instance, physDevice);

  vw.resize(200, 200);
  vw.show();

  return qapp.exec();
}
