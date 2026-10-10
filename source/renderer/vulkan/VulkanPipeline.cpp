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
#include <algorithm>
#include <functional>

namespace Duel6 {

    // VulkanPipeline implementation
    VulkanPipeline::VulkanPipeline(const VulkanDevice& device, VkRenderPass renderPass)
        : device(&device), pipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE), renderPass(renderPass) {
    }

    VulkanPipeline::~VulkanPipeline() {
        cleanup();
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

    VulkanPipelineBuilder& VulkanPipelineBuilder::setStencilTest(bool enable, VkCompareOp compareOp,
                                                                  VkStencilOp failOp, VkStencilOp passOp,
                                                                  VkStencilOp depthFailOp) {
        stencilTestEnable = enable;
        stencilCompareOp = compareOp;
        stencilFailOp = failOp;
        stencilPassOp = passOp;
        stencilDepthFailOp = depthFailOp;
        return *this;
    }

    VulkanPipelineBuilder& VulkanPipelineBuilder::setColorWriteMask(VkColorComponentFlags mask) {
        colorWriteMask = mask;
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
        depthStencil.stencilTestEnable = stencilTestEnable ? VK_TRUE : VK_FALSE;
        if (stencilTestEnable) {
            VkStencilOpState stencilOps{};
            stencilOps.failOp = stencilFailOp;
            stencilOps.passOp = stencilPassOp;
            stencilOps.depthFailOp = stencilDepthFailOp;
            stencilOps.compareOp = stencilCompareOp;
            stencilOps.compareMask = 0;  // dynamic
            stencilOps.writeMask = 0;    // dynamic
            stencilOps.reference = 0;    // dynamic
            depthStencil.front = stencilOps;
            depthStencil.back = stencilOps;
        }

        // Color blend state
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = colorWriteMask;
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
        frontFace = VK_FRONT_FACE_CLOCKWISE;  // Clockwise because negative viewport height flips winding
        wireframe = false;
        stencilTestEnable = false;
        stencilCompareOp = VK_COMPARE_OP_NOT_EQUAL;
        stencilFailOp = VK_STENCIL_OP_KEEP;
        stencilPassOp = VK_STENCIL_OP_KEEP;
        stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
        namespace {
            // Stencil ops are pipeline state; reference and masks are set per draw (dynamic)
            void configureStencilMode(VulkanPipelineBuilder& builder, uint32_t stencilMode) {
                builder.addDynamicState(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
                builder.addDynamicState(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
                builder.addDynamicState(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
                switch (stencilMode) {
                    case 1: // WRITE: mark pixels with the reference value
                        builder.setStencilTest(true, VK_COMPARE_OP_NOT_EQUAL,
                                               VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE);
                        break;
                    case 2: // TEST: draw only where not marked
                        builder.setStencilTest(true, VK_COMPARE_OP_NOT_EQUAL,
                                               VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP);
                        break;
                    case 3: // ERASE: clear the marks, no color output
                        builder.setStencilTest(true, VK_COMPARE_OP_NOT_EQUAL,
                                               VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_REPLACE);
                        builder.setColorWriteMask(0);
                        break;
                    default:
                        builder.setStencilTest(false);
                        break;
                }
            }

            // Everything the two pipelines share: shaders, vertex format, push constants, texture binding.
            // The shader objects must outlive build().
            // OpenGL geometry is clockwise (glFrontFace(GL_CW)); the y flip of the main pass keeps it so,
            // the upside-down offscreen pass inverts it
            VulkanPipelineBuilder& configureCommon(VulkanPipelineBuilder& builder, const VulkanShader& vertexShader,
                                                   const VulkanShader& fragmentShader, VkDescriptorSetLayout textureLayout,
                                                   VkFrontFace frontFace) {
                return builder
                    .addShader(vertexShader)
                    .addShader(fragmentShader)
                    .setVertexInput<VulkanVertex>()
                    .setCullMode(VK_CULL_MODE_BACK_BIT, frontFace)
                    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, PUSH_CONSTANTS_SIZE)
                    .addDescriptorSetLayout(textureLayout)
                    .setViewportDynamic()
                    .setScissorDynamic()
                    .addDynamicState(VK_DYNAMIC_STATE_LINE_WIDTH);
            }
        }

        std::unique_ptr<VulkanPipeline> createPipeline(const VulkanDevice& device, VkRenderPass renderPass,
                                                       VkDescriptorSetLayout textureLayout, VkPrimitiveTopology topology,
                                                       bool depthTest, bool depthWrite, BlendFunc blendFunc,
                                                       bool wireframe, uint32_t stencilMode, VkFrontFace frontFace) {
            VulkanPipelineBuilder builder(device, renderPass);
            VulkanShaderManager shaders(device, "shaders/vulkan");
            auto vertexShader = shaders.loadShader("main.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaders.loadShader("main.frag", VulkanShader::Type::Fragment);
            configureCommon(builder, *vertexShader, *fragmentShader, textureLayout, frontFace)
                .setTopology(topology)
                .setDepthTest(depthTest, depthWrite)  // VK_COMPARE_OP_LESS, like the OpenGL default
                .setWireframe(wireframe);
            configureStencilMode(builder, stencilMode);

            switch (blendFunc) {
                case BlendFunc::None:
                    builder.setBlending(false);
                    break;
                case BlendFunc::SrcAlpha:
                    builder.setBlending(true,
                        VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
                    break;
                case BlendFunc::SrcColor:
                    builder.setBlending(true,
                        VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD,
                        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
                    break;
            }
            return builder.build();
        }

        std::unique_ptr<VulkanPipeline> createDepthBlitPipeline(const VulkanDevice& device, VkRenderPass renderPass,
                                                                VkDescriptorSetLayout textureLayout) {
            VulkanPipelineBuilder builder(device, renderPass);
            VulkanShaderManager shaders(device, "shaders/vulkan");
            auto vertexShader = shaders.loadShader("main.vert", VulkanShader::Type::Vertex);
            auto fragmentShader = shaders.loadShader("depth_blit.frag", VulkanShader::Type::Fragment);
            return configureCommon(builder, *vertexShader, *fragmentShader, textureLayout, VK_FRONT_FACE_CLOCKWISE)
                .setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setDepthTest(true, true, VK_COMPARE_OP_ALWAYS)
                .setBlending(false)
                .setColorWriteMask(0)
                .build();
        }
    }

}
