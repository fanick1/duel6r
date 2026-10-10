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

#ifndef DUEL6_RENDERER_VULKANCONTEXT_H
#define DUEL6_RENDERER_VULKANCONTEXT_H

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>
#include <string>
#include "../../Type.h"
#include "../../Exception.h"
#include "../../console/Console.h"

namespace Duel6 {
    class VulkanException : public Exception {
    public:
        VulkanException(const std::string& file, Int32 line, const std::string& message) 
            : Exception(file, line, message) {}
    };

    class VulkanContext {
    private:
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        bool enableValidationLayers;
        std::vector<const char*> validationLayers;
        Console* console;

    public:
        explicit VulkanContext(Console& console);
        ~VulkanContext();

        // Non-copyable
        VulkanContext(const VulkanContext&) = delete;
        VulkanContext& operator=(const VulkanContext&) = delete;

        VkInstance getInstance() const { return instance; }

        void initialize(SDL_Window* window);
        void cleanup();

        std::vector<VkPhysicalDevice> enumeratePhysicalDevices() const;
        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        VkPhysicalDevice selectPhysicalDevice(VkSurfaceKHR surface) const;

    private:
        void setupValidationLayers();
        void createInstance(SDL_Window* window);
        void setupDebugMessenger();
        bool checkValidationLayerSupport() const;
        std::vector<const char*> getRequiredExtensions(SDL_Window* window) const;
        
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
    };
}

#endif