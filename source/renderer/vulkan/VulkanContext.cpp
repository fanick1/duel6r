/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "VulkanContext.h"
#include "../../Format.h"
#include <set>
#include <cstring>
#include <iostream>

namespace Duel6 {
    VulkanContext::VulkanContext(Console& console)
        : instance(VK_NULL_HANDLE)
        , debugMessenger(VK_NULL_HANDLE)
        , enableValidationLayers(true) // Enable debug
        , console(&console) {
        
        validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

#ifdef D6_DEBUG
        enableValidationLayers = true;
#endif
    }

    VulkanContext::~VulkanContext() {
        cleanup();
    }

    void VulkanContext::initialize(SDL_Window* window) {
        setupValidationLayers();
        createInstance(window);
        setupDebugMessenger();
        
        console->printLine("...Vulkan context initialized successfully");
    }

    void VulkanContext::cleanup() {
        if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, nullptr);
            }
            debugMessenger = VK_NULL_HANDLE;
        }

        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    void VulkanContext::setupValidationLayers() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            std::cerr << "Warning: Validation layers requested but not available";
            console->printLine("Warning: Validation layers requested but not available");
            enableValidationLayers = false;
        }

        if (enableValidationLayers) {
            std::cerr << "...Vulkan validation layers enabled";
            console->printLine("...Vulkan validation layers enabled");
        }
    }

    void VulkanContext::createInstance(SDL_Window* window) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Duel 6 Reloaded";
        appInfo.applicationVersion = VK_MAKE_VERSION(6, 0, 0);
        appInfo.pEngineName = "Duel6Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions(window);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;
            debugCreateInfo.pUserData = console;

            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create Vulkan instance, error: {0}") << result);
        }

        console->printLine("...Vulkan instance created");
    }

    void VulkanContext::setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = console;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to set up debug messenger, error: {0}") << result);
            }
        } else {
            D6_THROW(VulkanException, "vkCreateDebugUtilsMessengerEXT function not found");
        }
    }

    bool VulkanContext::checkValidationLayerSupport() const {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanContext::getRequiredExtensions(SDL_Window* window) const {
        // Get SDL required extensions
        unsigned int sdlExtensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr)) {
            D6_THROW(VulkanException, Format("Failed to get SDL Vulkan extension count: {0}") << SDL_GetError());
        }

        std::vector<const char*> extensions(sdlExtensionCount);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, extensions.data())) {
            D6_THROW(VulkanException, Format("Failed to get SDL Vulkan extensions: {0}") << SDL_GetError());
        }

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        console->printLine(Format("...Vulkan extensions required: {0}") << extensions.size());
        for (const auto& ext : extensions) {
            console->printLine(Format("   {0}") << ext);
        }

        return extensions;
    }

    std::vector<VkPhysicalDevice> VulkanContext::enumeratePhysicalDevices() const {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            D6_THROW(VulkanException, "Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        console->printLine(Format("...Found {0} Vulkan-compatible devices") << deviceCount);
        return devices;
    }

    bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Basic requirements check
        bool suitable = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                       deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

        // Check for required device extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        suitable = suitable && requiredExtensions.empty();

        // Check swapchain support
        if (suitable) {
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            suitable = formatCount > 0 && presentModeCount > 0;
        }

        return suitable;
    }

    VkPhysicalDevice VulkanContext::selectPhysicalDevice(VkSurfaceKHR surface) const {
        auto devices = enumeratePhysicalDevices();
        
        // Score devices and pick the best one
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        int highestScore = -1;

        for (const auto& device : devices) {
            if (!isDeviceSuitable(device, surface)) {
                continue;
            }

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            int score = 0;
            
            // Discrete GPUs have a significant advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score += 100;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;

            console->printLine(Format("...Device: {0} (Score: {1})") 
                << deviceProperties.deviceName << score);

            if (score > highestScore) {
                highestScore = score;
                bestDevice = device;
            }
        }

        if (bestDevice == VK_NULL_HANDLE) {
            D6_THROW(VulkanException, "Failed to find a suitable GPU");
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(bestDevice, &deviceProperties);
        console->printLine(Format("...Selected GPU: {0}") << deviceProperties.deviceName);

        return bestDevice;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        
        Console* console = static_cast<Console*>(pUserData);
        
        std::string prefix;
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            prefix = "[VK ERROR]";
        } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            prefix = "[VK WARN]";
        } else {
            prefix = "[VK INFO]";
        }

        if (console) {
            std::cerr << prefix << " " << pCallbackData->pMessage << "\n";
            console->printLine(Format("{0} {1}") << prefix << pCallbackData->pMessage);
        }

        return VK_FALSE;
    }
}