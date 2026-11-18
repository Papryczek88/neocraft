#include <windows.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kWindowWidth = 1280;
constexpr std::uint32_t kWindowHeight = 720;
constexpr const char* kWindowClassName = "NeoCraftVulkanWindow";

void checkVk(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " (VkResult " + std::to_string(result) + ")");
    }
}

struct QueueFamily {
    std::uint32_t index{};
    bool found{};
};

class VulkanApp {
public:
    int run() {
        createWindow();
        createInstance();
        createSurface();
        selectPhysicalDevice();
        createDevice();
        createSwapchainResources();
        createSyncObjects();
        eventLoop();
        return 0;
    }

    ~VulkanApp() {
        cleanup();
    }

private:
    HINSTANCE instanceHandle_ = GetModuleHandleW(nullptr);
    HWND window_{};
    bool resized_{};
    bool minimized_{};
    VkInstance vkInstance_{};
    VkSurfaceKHR surface_{};
    VkPhysicalDevice physicalDevice_{};
    std::uint32_t queueFamilyIndex_{};
    VkDevice device_{};
    VkQueue queue_{};
    VkSwapchainKHR swapchain_{};
    VkFormat swapchainFormat_{};
    VkExtent2D swapchainExtent_{};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> imageViews_;
    VkRenderPass renderPass_{};
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool commandPool_{};
    std::vector<VkCommandBuffer> commandBuffers_;
    VkSemaphore imageAvailable_{};
    VkSemaphore renderFinished_{};
    VkFence inFlightFence_{};

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* app = reinterpret_cast<VulkanApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            app = static_cast<VulkanApp*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        if (app != nullptr) {
            if (message == WM_SIZE) {
                app->minimized_ = (wParam == SIZE_MINIMIZED);
                app->resized_ = true;
            } else if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
                PostMessageW(window, WM_CLOSE, 0, 0);
            }
        }
        if (message == WM_CLOSE) {
            DestroyWindow(window);
            return 0;
        }
        if (message == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    void createWindow() {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = instanceHandle_;
        windowClass.lpszClassName = L"NeoCraftVulkanWindow";
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        if (RegisterClassW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            throw std::runtime_error("Nie można zarejestrować klasy okna Win32.");
        }

        RECT rect{0, 0, static_cast<LONG>(kWindowWidth), static_cast<LONG>(kWindowHeight)};
        if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
            throw std::runtime_error("Nie można obliczyć rozmiaru okna.");
        }
        window_ = CreateWindowExW(
            0, windowClass.lpszClassName, L"NeoCraft 1.0 — Vulkan",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, instanceHandle_, this);
        if (window_ == nullptr) {
            throw std::runtime_error("Nie można utworzyć okna Win32.");
        }
        ShowWindow(window_, SW_SHOW);
    }

    void createInstance() {
        constexpr std::array<const char*, 2> extensions{
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        applicationInfo.pApplicationName = "NeoCraft";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pEngineName = "NeoCraft Engine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        applicationInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        createInfo.pApplicationInfo = &applicationInfo;
        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        checkVk(vkCreateInstance(&createInfo, nullptr, &vkInstance_), "vkCreateInstance");
    }

    void createSurface() {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        surfaceInfo.hinstance = instanceHandle_;
        surfaceInfo.hwnd = window_;
        checkVk(vkCreateWin32SurfaceKHR(vkInstance_, &surfaceInfo, nullptr, &surface_), "vkCreateWin32SurfaceKHR");
    }

    bool supportsSwapchain(VkPhysicalDevice candidate) const {
        std::uint32_t count = 0;
        if (vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, nullptr) != VK_SUCCESS) {
            return false;
        }
        std::vector<VkExtensionProperties> extensions(count);
        if (vkEnumerateDeviceExtensionProperties(candidate, nullptr, &count, extensions.data()) != VK_SUCCESS) {
            return false;
        }
        return std::any_of(extensions.begin(), extensions.end(), [](const VkExtensionProperties& extension) {
            return std::string(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        });
    }

    void selectPhysicalDevice() {
        std::uint32_t count = 0;
        checkVk(vkEnumeratePhysicalDevices(vkInstance_, &count, nullptr), "vkEnumeratePhysicalDevices");
        if (count == 0) {
            throw std::runtime_error("Nie znaleziono urządzenia Vulkan. Sprawdź sterownik GPU i Vulkan SDK.");
        }
        std::vector<VkPhysicalDevice> devices(count);
        checkVk(vkEnumeratePhysicalDevices(vkInstance_, &count, devices.data()), "vkEnumeratePhysicalDevices");

        for (VkPhysicalDevice candidate : devices) {
            if (!supportsSwapchain(candidate)) {
                continue;
            }
            std::uint32_t familyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
            for (std::uint32_t i = 0; i < familyCount; ++i) {
                VkBool32 presentSupported = VK_FALSE;
                const VkResult presentResult = vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface_, &presentSupported);
                if (presentResult == VK_SUCCESS && presentSupported &&
                    (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && families[i].queueCount > 0) {
                    physicalDevice_ = candidate;
                    queueFamilyIndex_ = i;
                    VkPhysicalDeviceProperties properties{};
                    vkGetPhysicalDeviceProperties(candidate, &properties);
                    std::cout << "Urządzenie Vulkan: " << properties.deviceName << '\n';
                    return;
                }
            }
        }
        throw std::runtime_error("Nie znaleziono GPU z kolejką grafiki, prezentacji i obsługą swapchain.");
    }

    void createDevice() {
        const float priority = 1.0F;
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = queueFamilyIndex_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        constexpr const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkPhysicalDeviceFeatures features{};
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = 1;
        deviceInfo.ppEnabledExtensionNames = extensions;
        deviceInfo.pEnabledFeatures = &features;
        checkVk(vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_), "vkCreateDevice");
        vkGetDeviceQueue(device_, queueFamilyIndex_, 0, &queue_);

        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex_;
        checkVk(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "vkCreateCommandPool");
    }

    void createSwapchainResources() {
        VkSurfaceCapabilitiesKHR capabilities{};
        checkVk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        std::uint32_t formatCount = 0;
        checkVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        if (formatCount == 0) {
            throw std::runtime_error("Powierzchnia Vulkan nie udostępnia formatów obrazu.");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        checkVk(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        VkSurfaceFormatKHR surfaceFormat = formats.front();
        for (const auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = format;
                break;
            }
        }
        if (surfaceFormat.format == VK_FORMAT_UNDEFINED) {
            surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
        }
        swapchainFormat_ = surfaceFormat.format;

        if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
            swapchainExtent_ = capabilities.currentExtent;
        } else {
            RECT client{};
            GetClientRect(window_, &client);
            const auto width = static_cast<std::uint32_t>(std::max<LONG>(client.right - client.left, 1));
            const auto height = static_cast<std::uint32_t>(std::max<LONG>(client.bottom - client.top, 1));
            swapchainExtent_.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            swapchainExtent_.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        std::uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> alphaOptions{
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };
        for (const auto option : alphaOptions) {
            if ((capabilities.supportedCompositeAlpha & option) != 0) {
                compositeAlpha = option;
                break;
            }
        }

        VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainInfo.surface = surface_;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = swapchainExtent_;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = capabilities.currentTransform;
        swapchainInfo.compositeAlpha = compositeAlpha;
        swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        checkVk(vkCreateSwapchainKHR(device_, &swapchainInfo, nullptr, &swapchain_), "vkCreateSwapchainKHR");

        checkVk(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr), "vkGetSwapchainImagesKHR");
        swapchainImages_.resize(imageCount);
        checkVk(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()), "vkGetSwapchainImagesKHR");
        imageViews_.reserve(swapchainImages_.size());
        for (const auto image : swapchainImages_) {
            VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainFormat_;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            VkImageView view{};
            checkVk(vkCreateImageView(device_, &viewInfo, nullptr, &view), "vkCreateImageView");
            imageViews_.push_back(view);
        }

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapchainFormat_;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        checkVk(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_), "vkCreateRenderPass");

        framebuffers_.reserve(imageViews_.size());
        for (const auto view : imageViews_) {
            VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebufferInfo.renderPass = renderPass_;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &view;
            framebufferInfo.width = swapchainExtent_.width;
            framebufferInfo.height = swapchainExtent_.height;
            framebufferInfo.layers = 1;
            VkFramebuffer framebuffer{};
            checkVk(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &framebuffer), "vkCreateFramebuffer");
            framebuffers_.push_back(framebuffer);
        }

        commandBuffers_.resize(framebuffers_.size());
        VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocation.commandPool = commandPool_;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = static_cast<std::uint32_t>(commandBuffers_.size());
        checkVk(vkAllocateCommandBuffers(device_, &allocation, commandBuffers_.data()), "vkAllocateCommandBuffers");
        for (std::size_t i = 0; i < commandBuffers_.size(); ++i) {
            VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            checkVk(vkBeginCommandBuffer(commandBuffers_[i], &beginInfo), "vkBeginCommandBuffer");
            const VkClearValue clearColor{{{0.12F, 0.30F, 0.25F, 1.0F}}};
            VkRenderPassBeginInfo renderBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            renderBegin.renderPass = renderPass_;
            renderBegin.framebuffer = framebuffers_[i];
            renderBegin.renderArea.extent = swapchainExtent_;
            renderBegin.clearValueCount = 1;
            renderBegin.pClearValues = &clearColor;
            vkCmdBeginRenderPass(commandBuffers_[i], &renderBegin, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(commandBuffers_[i]);
            checkVk(vkEndCommandBuffer(commandBuffers_[i]), "vkEndCommandBuffer");
        }
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        checkVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailable_), "vkCreateSemaphore(imageAvailable)");
        checkVk(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinished_), "vkCreateSemaphore(renderFinished)");
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        checkVk(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence_), "vkCreateFence");
    }

    void recreateSwapchain() {
        RECT client{};
        while (GetClientRect(window_, &client) && (client.right == client.left || client.bottom == client.top)) {
            WaitMessage();
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        checkVk(vkDeviceWaitIdle(device_), "vkDeviceWaitIdle");
        if (!commandBuffers_.empty()) {
            vkFreeCommandBuffers(device_, commandPool_, static_cast<std::uint32_t>(commandBuffers_.size()), commandBuffers_.data());
            commandBuffers_.clear();
        }
        destroySwapchainResources();
        createSwapchainResources();
        resized_ = false;
    }

    void drawFrame() {
        checkVk(vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, UINT64_MAX), "vkWaitForFences");
        std::uint32_t imageIndex = 0;
        const VkResult acquire = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailable_, VK_NULL_HANDLE, &imageIndex);
        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        }
        if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
            checkVk(acquire, "vkAcquireNextImageKHR");
        }
        checkVk(vkResetFences(device_, 1, &inFlightFence_), "vkResetFences");
        const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailable_;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffers_[imageIndex];
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinished_;
        checkVk(vkQueueSubmit(queue_, 1, &submit, inFlightFence_), "vkQueueSubmit");

        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinished_;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain_;
        present.pImageIndices = &imageIndex;
        const VkResult result = vkQueuePresentKHR(queue_, &present);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized_) {
            recreateSwapchain();
        } else {
            checkVk(result, "vkQueuePresentKHR");
        }
    }

    void eventLoop() {
        MSG message{};
        bool running = true;
        while (running) {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    running = false;
                    break;
                }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            if (!running) {
                break;
            }
            if (minimized_) {
                WaitMessage();
                continue;
            }
            drawFrame();
        }
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
        }
    }

    void destroySwapchainResources() noexcept {
        if (device_ == VK_NULL_HANDLE) return;
        for (const auto framebuffer : framebuffers_) vkDestroyFramebuffer(device_, framebuffer, nullptr);
        framebuffers_.clear();
        if (renderPass_ != VK_NULL_HANDLE) vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
        for (const auto view : imageViews_) vkDestroyImageView(device_, view, nullptr);
        imageViews_.clear();
        swapchainImages_.clear();
        if (swapchain_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }

    void cleanup() noexcept {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
            if (!commandBuffers_.empty() && commandPool_ != VK_NULL_HANDLE) {
                vkFreeCommandBuffers(device_, commandPool_, static_cast<std::uint32_t>(commandBuffers_.size()), commandBuffers_.data());
            }
            destroySwapchainResources();
            if (imageAvailable_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, imageAvailable_, nullptr);
            if (renderFinished_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderFinished_, nullptr);
            if (inFlightFence_ != VK_NULL_HANDLE) vkDestroyFence(device_, inFlightFence_, nullptr);
            if (commandPool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, commandPool_, nullptr);
            vkDestroyDevice(device_, nullptr);
        }
        if (surface_ != VK_NULL_HANDLE && vkInstance_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(vkInstance_, surface_, nullptr);
        if (vkInstance_ != VK_NULL_HANDLE) vkDestroyInstance(vkInstance_, nullptr);
        if (window_ != nullptr) {
            DestroyWindow(window_);
            window_ = nullptr;
        }
    }
};

} // namespace

int runVulkanApp() {
    VulkanApp app;
    return app.run();
}
