/*
* Copyright (c) 2006-2025, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice,
*       this list of conditions and the following disclaimer in the documentation
*       and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the names of its contributors may be
*       used to endorse or promote products derived from this software without
*       specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DUEL6_RENDERER_VULKAN_SHADER_H
#define DUEL6_RENDERER_VULKAN_SHADER_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace Duel6 {

    // Forward declarations
    class VulkanDevice;

    /**
     * Vulkan shader module wrapper
     */
    class VulkanShader {
    public:
        enum class Type {
            Vertex = VK_SHADER_STAGE_VERTEX_BIT,
            Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
            Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
            Compute = VK_SHADER_STAGE_COMPUTE_BIT
        };

    private:
        const VulkanDevice* device;
        VkShaderModule shaderModule;
        Type shaderType;
        std::string entryPoint;

    public:
        /**
         * Create shader from SPIR-V binary data
         */
        VulkanShader(const VulkanDevice& device, Type type, const std::vector<uint32_t>& spirvCode, const std::string& entryPoint = "main");
        
        /**
         * Create shader from SPIR-V file
         */
        VulkanShader(const VulkanDevice& device, Type type, const std::string& spirvFilePath, const std::string& entryPoint = "main");
        
        ~VulkanShader();

        // Non-copyable
        VulkanShader(const VulkanShader&) = delete;
        VulkanShader& operator=(const VulkanShader&) = delete;

        // Movable
        VulkanShader(VulkanShader&& other) noexcept;
        VulkanShader& operator=(VulkanShader&& other) noexcept;

        // Getters
        VkShaderModule getModule() const { return shaderModule; }
        Type getType() const { return shaderType; }
        VkShaderStageFlagBits getStageFlags() const { return static_cast<VkShaderStageFlagBits>(shaderType); }
        const std::string& getEntryPoint() const { return entryPoint; }

        // Create pipeline shader stage info
        VkPipelineShaderStageCreateInfo getStageCreateInfo() const;

    private:
        void createShaderModule(const std::vector<uint32_t>& spirvCode);
        void cleanup();

        // Helper function to load SPIR-V from file
        static std::vector<uint32_t> loadSpirvFromFile(const std::string& filePath);
    };

    /**
     * Shader manager for handling shader compilation and caching
     */
    class VulkanShaderManager {
    private:
        const VulkanDevice* device;
        std::string shaderDirectory;

    public:
        explicit VulkanShaderManager(const VulkanDevice& device, const std::string& shaderDir = "shaders");
        ~VulkanShaderManager() = default;

        // Non-copyable
        VulkanShaderManager(const VulkanShaderManager&) = delete;
        VulkanShaderManager& operator=(const VulkanShaderManager&) = delete;

        /**
         * Load shader from SPIR-V file
         * @param filename Base filename without extension (e.g., "color.vert")
         * @param type Shader type
         * @return Unique pointer to loaded shader
         */
        std::unique_ptr<VulkanShader> loadShader(const std::string& filename, VulkanShader::Type type);

        /**
         * Create shader pair for vertex + fragment shaders
         * @param baseName Base name (e.g., "color" for "color.vert.spv" and "color.frag.spv")
         * @return Pair of vertex and fragment shaders
         */
        std::pair<std::unique_ptr<VulkanShader>, std::unique_ptr<VulkanShader>> 
        createShaderPair(const std::string& baseName);

        /**
         * Set shader directory path
         */
        void setShaderDirectory(const std::string& directory) { shaderDirectory = directory; }

        /**
         * Get full path to shader file
         */
        std::string getShaderPath(const std::string& filename) const;

    private:
        std::string getShaderExtension(VulkanShader::Type type) const;
    };

}

#endif
