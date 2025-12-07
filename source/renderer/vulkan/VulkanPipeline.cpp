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

#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanShader.h"
#include "VulkanTypes.h"
#include "../../Exception.h"
#include "../../VideoException.h"
#include <array>
#include <algorithm>
#include <fstream>
#include <functional>

namespace Duel6 {

    // VulkanPipeline implementation
    VulkanPipeline::VulkanPipeline(const VulkanDevice& device, VkRenderPass renderPass)
        : device(&device), pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), renderPass(renderPass) {
    }

    VulkanPipeline::~VulkanPipeline() {
        cleanup();
    }

    VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept
        : device(other.device), pipeline(other.pipeline), pipelineLayout(other.pipelineLayout),
          renderPass(other.renderPass), descriptorSetLayouts(std::move(other.descriptorSetLayouts)) {
        other.device = nullptr;
        other.pipeline = VK_NULL_HANDLE;
        other.pipelineLayout = VK_NULL_HANDLE;
    }

    VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            device = other.device;
            pipeline = other.pipeline;
            pipelineLayout = other.pipelineLayout;
            renderPass = other.renderPass;
            descriptorSetLayouts = std::move(other.descriptorSetLayouts);
            
            other.device = nullptr;
            other.pipeline = VK_NULL_HANDLE;
            other.pipelineLayout = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VulkanPipeline::bind(VkCommandBuffer commandBuffer) const {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void VulkanPipeline::cleanup() {
        if (device != nullptr) {
            if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device->getLogicalDevice(), pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device->getLogicalDevice(), pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
        }
    }

    // VulkanPipelineBuilder implementation
    VulkanPipelineBuilder::VulkanPipelineBuilder(const VulkanDevice& device, VkRenderPass renderPass)
        : device(&device), renderPass(renderPass), hasVertexInput(false) {
        setDefaults();
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addShader(const VulkanShader& shader) {
        shaderStages.push_back(shader.getStageCreateInfo());
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addShaderStage(VkShaderStageFlagBits stage, VkShaderModule module, const char* entryPoint) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = entryPoint;
        shaderStages.push_back(stageInfo);
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setVertexInput(const VkVertexInputBindingDescription& binding,
                                                                 const std::vector<VkVertexInputAttributeDescription>& attributes) {
        vertexBinding = binding;
        vertexAttributes = attributes;
        hasVertexInput = true;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setTopology(VkPrimitiveTopology topology) {
        this->topology = topology;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setDepthTest(bool enable, bool writeEnable, VkCompareOp compareOp) {
        depthTestEnable = enable;
        depthWriteEnable = writeEnable;
        depthCompareOp = compareOp;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setBlending(bool enable, 
                                                              VkBlendFactor srcColor, VkBlendFactor dstColor, VkBlendOp colorOp,
                                                              VkBlendFactor srcAlpha, VkBlendFactor dstAlpha, VkBlendOp alphaOp) {
        blendEnable = enable;
        srcColorBlendFactor = srcColor;
        dstColorBlendFactor = dstColor;
        colorBlendOp = colorOp;
        srcAlphaBlendFactor = srcAlpha;
        dstAlphaBlendFactor = dstAlpha;
        alphaBlendOp = alphaOp;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
        this->cullMode = cullMode;
        this->frontFace = frontFace;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setWireframe(bool enable) {
        wireframe = enable;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
        descriptorSetLayouts.push_back(layout);
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size) {
        VkPushConstantRange range{};
        range.stageFlags = stageFlags;
        range.offset = offset;
        range.size = size;
        pushConstantRanges.push_back(range);
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::addDynamicState(VkDynamicState state) {
        // Prevent duplicates
        if (std::find(dynamicStates.begin(), dynamicStates.end(), state) == dynamicStates.end()) {
            dynamicStates.push_back(state);
        }
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setViewportDynamic() {
        return addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setScissorDynamic() {
        return addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
    }

    std::unique_ptr<VulkanPipeline> VulkanPipelineBuilder::build() {
        auto pipeline = std::make_unique<VulkanPipeline>(*device, renderPass);
        pipeline->descriptorSetLayouts = descriptorSetLayouts;

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

        VkResult result = vkCreatePipelineLayout(device->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipeline->pipelineLayout);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create pipeline layout: " + std::to_string(result));
        }

        // Vertex input state
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        if (hasVertexInput) {
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &vertexBinding;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
            vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();
        }

        // Input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport state (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = cullMode;
        rasterizer.frontFace = frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;

        // Multisampling state
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth and stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = depthTestEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = depthCompareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // Color blend state
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
        colorBlendAttachment.colorBlendOp = colorBlendOp;
        colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
        colorBlendAttachment.alphaBlendOp = alphaBlendOp;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Dynamic state
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipeline->pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(device->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->pipeline);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create graphics pipeline: " + std::to_string(result));
        }

        return pipeline;
    }

    void VulkanPipelineBuilder::setDefaults() {
        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        depthTestEnable = true;
        depthWriteEnable = true;
        depthCompareOp = VK_COMPARE_OP_LESS;
        blendEnable = false;
        srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendOp = VK_BLEND_OP_ADD;
        srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        alphaBlendOp = VK_BLEND_OP_ADD;
        cullMode = VK_CULL_MODE_BACK_BIT;
        frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        wireframe = false;

        // Default dynamic states removed - they will be added by setViewportDynamic/setScissorDynamic calls
    }

    // VulkanPipelineCache implementation
    VulkanPipelineCache::VulkanPipelineCache(const VulkanDevice& device)
        : device(&device), pipelineCache(VK_NULL_HANDLE) {
        
        VkPipelineCacheCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        
        VkResult result = vkCreatePipelineCache(device.getLogicalDevice(), &createInfo, nullptr, &pipelineCache);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create pipeline cache: " + std::to_string(result));
        }
    }

    VulkanPipelineCache::~VulkanPipelineCache() {
        cleanup();
    }

    VulkanPipeline* VulkanPipelineCache::getOrCreatePipeline(size_t hash, const std::function<std::unique_ptr<VulkanPipeline>()>& creator) {
        auto it = cachedPipelines.find(hash);
        if (it != cachedPipelines.end()) {
            return it->second.get();
        }
        
        auto pipeline = creator();
        VulkanPipeline* pipelinePtr = pipeline.get();
        cachedPipelines[hash] = std::move(pipeline);
        return pipelinePtr;
    }

    void VulkanPipelineCache::saveCacheToFile(const std::string& filename) const {
        size_t dataSize = 0;
        VkResult result = vkGetPipelineCacheData(device->getLogicalDevice(), pipelineCache, &dataSize, nullptr);
        if (result != VK_SUCCESS || dataSize == 0) {
            return;
        }

        std::vector<uint8_t> data(dataSize);
        result = vkGetPipelineCacheData(device->getLogicalDevice(), pipelineCache, &dataSize, data.data());
        if (result != VK_SUCCESS) {
            return;
        }

        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(data.data()), dataSize);
        }
    }

    void VulkanPipelineCache::loadCacheFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint8_t> data(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);
        file.close();

        // Recreate pipeline cache with loaded data
        cleanup();

        VkPipelineCacheCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        createInfo.initialDataSize = fileSize;
        createInfo.pInitialData = data.data();

        VkResult result = vkCreatePipelineCache(device->getLogicalDevice(), &createInfo, nullptr, &pipelineCache);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create pipeline cache from file: " + std::to_string(result));
        }
    }

    void VulkanPipelineCache::cleanup() {
        if (pipelineCache != VK_NULL_HANDLE && device != nullptr) {
            vkDestroyPipelineCache(device->getLogicalDevice(), pipelineCache, nullptr);
            pipelineCache = VK_NULL_HANDLE;
        }
        cachedPipelines.clear();
    }

    /**
     * Pipeline Presets - Common pipeline configurations
     */
    namespace PipelinePresets {

        std::unique_ptr<VulkanPipeline> createColorPipeline(const VulkanDevice& device, 
                                                            VkRenderPass renderPass,
                                                            VkDescriptorSetLayout uniformLayout) {
            VulkanPipelineBuilder builder(device, renderPass);
            
            // Load vertex and fragment shaders for colored geometry
            VulkanShaderManager shaderManager(device, "shaders/vulkan");
            auto vertexShader = shaderManager.loadShader("color.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaderManager.loadShader("color.frag", VulkanShader::Type::Fragment);
            
            return builder
                .addShader(*vertexShader)
                .addShader(*fragmentShader)
                .setVertexInput<VertexFormats::ColorVertex>()
                .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setDepthTest(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
                .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .addDescriptorSetLayout(uniformLayout)
                .setViewportDynamic()
                .setScissorDynamic()
                .build();
        }

        std::unique_ptr<VulkanPipeline> createSimpleColorPipeline(const VulkanDevice& device, 
                                                                   VkRenderPass renderPass) {
            VulkanPipelineBuilder builder(device, renderPass);
            
            // Load simple_color shaders that use push constants instead of UBO
            VulkanShaderManager shaderManager(device, "shaders/vulkan");
            auto vertexShader = shaderManager.loadShader("simple_color.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaderManager.loadShader("simple_color.frag", VulkanShader::Type::Fragment);
            
            return builder
                .addShader(*vertexShader)
                .addShader(*fragmentShader)
                .setVertexInput<VertexFormats::ColorVertex>()
                .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setDepthTest(false, false)  // Disable depth test for 2D UI
                .setCullMode(VK_CULL_MODE_NONE)  // No culling for 2D UI
                .setBlending(true)  // Enable alpha blending
                .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16)  // mat4 MVP
                .setViewportDynamic()
                .setScissorDynamic()
                .build();
        }

        std::unique_ptr<VulkanPipeline> createMaterialPipeline(const VulkanDevice& device, 
                                                               VkRenderPass renderPass,
                                                               VkDescriptorSetLayout uniformLayout,
                                                               VkDescriptorSetLayout textureLayout,
                                                               VkDescriptorSetLayout materialUniformLayout) {
            VulkanPipelineBuilder builder(device, renderPass);
            
            // Load vertex and fragment shaders for textured geometry
            VulkanShaderManager shaderManager(device, "shaders/vulkan");
            auto vertexShader = shaderManager.loadShader("material.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaderManager.loadShader("material.frag", VulkanShader::Type::Fragment);
            
            return builder
                .addShader(*vertexShader)
                .addShader(*fragmentShader)
                .setVertexInput<VertexFormats::MaterialVertex>()
                .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setDepthTest(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
                .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .addDescriptorSetLayout(uniformLayout)
                .addDescriptorSetLayout(textureLayout)
                .addDescriptorSetLayout(materialUniformLayout)
                .setViewportDynamic()
                .setScissorDynamic()
                .build();
        }

        std::unique_ptr<VulkanPipeline> createBloomFilterPipeline(const VulkanDevice& device, 
                                                                  VkRenderPass renderPass,
                                                                  VkDescriptorSetLayout uniformLayout,
                                                                  VkDescriptorSetLayout textureLayout,
                                                                  VkDescriptorSetLayout bloomUniformLayout) {
            VulkanPipelineBuilder builder(device, renderPass);
            
            // Load vertex and fragment shaders for bloom filter post-processing
            VulkanShaderManager shaderManager(device, "shaders/vulkan");
            auto vertexShader = shaderManager.loadShader("color.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaderManager.loadShader("bloom_filter.frag", VulkanShader::Type::Fragment);
            
            return builder
                .addShader(*vertexShader)
                .addShader(*fragmentShader)
                .setVertexInput<VertexFormats::ColorVertex>()  // Simple vertex for fullscreen quad
                .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setDepthTest(false, false, VK_COMPARE_OP_ALWAYS)
                .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .setBlending(true, 
                            VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                            VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD)
                .addDescriptorSetLayout(uniformLayout)
                .addDescriptorSetLayout(textureLayout)
                .addDescriptorSetLayout(bloomUniformLayout)
                .setViewportDynamic()
                .setScissorDynamic()
                .build();
        }

    }

}
