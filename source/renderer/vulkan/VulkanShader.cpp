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

#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "../../File.h"
#include "../../Exception.h"
#include "../../VideoException.h"
#include <fstream>
#include <iostream>

namespace Duel6 {

    // VulkanShader implementation
    VulkanShader::VulkanShader(const VulkanDevice& device, Type type, const std::vector<uint32_t>& spirvCode, const std::string& entryPoint)
        : device(&device), shaderModule(VK_NULL_HANDLE), shaderType(type), entryPoint(entryPoint) {
        createShaderModule(spirvCode);
    }

    VulkanShader::VulkanShader(const VulkanDevice& device, Type type, const std::string& spirvFilePath, const std::string& entryPoint)
        : device(&device), shaderModule(VK_NULL_HANDLE), shaderType(type), entryPoint(entryPoint) {
        auto spirvCode = loadSpirvFromFile(spirvFilePath);
        createShaderModule(spirvCode);
    }

    VulkanShader::~VulkanShader() {
        cleanup();
    }

    VulkanShader::VulkanShader(VulkanShader&& other) noexcept
        : device(other.device), shaderModule(other.shaderModule), 
          shaderType(other.shaderType), entryPoint(std::move(other.entryPoint)) {
        other.device = nullptr;
        other.shaderModule = VK_NULL_HANDLE;
    }

    VulkanShader& VulkanShader::operator=(VulkanShader&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            device = other.device;
            shaderModule = other.shaderModule;
            shaderType = other.shaderType;
            entryPoint = std::move(other.entryPoint);
            
            other.device = nullptr;
            other.shaderModule = VK_NULL_HANDLE;
        }
        return *this;
    }

    VkPipelineShaderStageCreateInfo VulkanShader::getStageCreateInfo() const {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = getStageFlags();
        stageInfo.module = shaderModule;
        stageInfo.pName = entryPoint.c_str();
        return stageInfo;
    }

    void VulkanShader::createShaderModule(const std::vector<uint32_t>& spirvCode) {
        if (spirvCode.empty()) {
            D6_THROW(VideoException, "SPIR-V code is empty");
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
        createInfo.pCode = spirvCode.data();

        VkResult result = vkCreateShaderModule(device->getLogicalDevice(), &createInfo, nullptr, &shaderModule);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create shader module: " + std::to_string(result));
        }
    }

    void VulkanShader::cleanup() {
        if (shaderModule != VK_NULL_HANDLE && device != nullptr) {
            vkDestroyShaderModule(device->getLogicalDevice(), shaderModule, nullptr);
            shaderModule = VK_NULL_HANDLE;
        }
    }

    std::vector<uint32_t> VulkanShader::loadSpirvFromFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);
        
        if (!file.is_open()) {
            D6_THROW(VideoException, "Failed to open shader file: " + filePath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        if (fileSize % sizeof(uint32_t) != 0) {
            D6_THROW(VideoException, "Invalid SPIR-V file size: " + filePath);
        }

        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close();

        return buffer;
    }

    // VulkanShaderManager implementation
    VulkanShaderManager::VulkanShaderManager(const VulkanDevice& device, const std::string& shaderDir)
        : device(&device), shaderDirectory(shaderDir) {
    }

    std::unique_ptr<VulkanShader> VulkanShaderManager::loadShader(const std::string& filename, VulkanShader::Type type) {
        std::string fullPath = getShaderPath(filename + getShaderExtension(type));
        return std::make_unique<VulkanShader>(*device, type, fullPath);
    }

    std::pair<std::unique_ptr<VulkanShader>, std::unique_ptr<VulkanShader>> 
    VulkanShaderManager::createShaderPair(const std::string& baseName) {
        auto vertexShader = loadShader(baseName + ".vert", VulkanShader::Type::Vertex);
        auto fragmentShader = loadShader(baseName + ".frag", VulkanShader::Type::Fragment);
        
        return std::make_pair(std::move(vertexShader), std::move(fragmentShader));
    }

    std::string VulkanShaderManager::getShaderPath(const std::string& filename) const {
        return shaderDirectory + "/" + filename;
    }

    std::string VulkanShaderManager::getShaderExtension(VulkanShader::Type type) const {
        switch (type) {
            case VulkanShader::Type::Vertex:
                return ".spv";
            case VulkanShader::Type::Fragment:
                return ".spv";
            case VulkanShader::Type::Geometry:
                return ".spv";
            case VulkanShader::Type::Compute:
                return ".spv";
            default:
                return ".spv";
        }
    }

}
