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

#ifndef DUEL6_RENDERER_VULKAN_PIPELINE_H
#define DUEL6_RENDERER_VULKAN_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "VulkanShader.h"

namespace Duel6 {

    // Forward declarations
    class VulkanDevice;
    class VulkanShader;

    /**
     * Vulkan graphics pipeline wrapper
     */
    class VulkanPipeline {
    private:
        const VulkanDevice* device;
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    public:
        VulkanPipeline(const VulkanDevice& device, VkRenderPass renderPass);
        ~VulkanPipeline();

        // Non-copyable
        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        // Movable
        VulkanPipeline(VulkanPipeline&& other) noexcept;
        VulkanPipeline& operator=(VulkanPipeline&& other) noexcept;

        // Getters
        VkPipeline getPipeline() const { return pipeline; }
        VkPipelineLayout getLayout() const { return pipelineLayout; }
        const std::vector<VkDescriptorSetLayout>& getDescriptorSetLayouts() const { return descriptorSetLayouts; }

        // Bind pipeline for rendering
        void bind(VkCommandBuffer commandBuffer) const;

    private:
        void cleanup();
        friend class VulkanPipelineBuilder;
    };

    /**
     * Builder for creating Vulkan graphics pipelines
     */
    class VulkanPipelineBuilder {
    private:
        const VulkanDevice* device;
        VkRenderPass renderPass;
        
        // Shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        
        // Vertex input
        VkVertexInputBindingDescription vertexBinding;
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;
        bool hasVertexInput;
        
        // Pipeline state
        VkPrimitiveTopology topology;
        bool depthTestEnable;
        bool depthWriteEnable;
        VkCompareOp depthCompareOp;
        bool blendEnable;
        VkBlendFactor srcColorBlendFactor;
        VkBlendFactor dstColorBlendFactor;
        VkBlendOp colorBlendOp;
        VkBlendFactor srcAlphaBlendFactor;
        VkBlendFactor dstAlphaBlendFactor;
        VkBlendOp alphaBlendOp;
        VkCullModeFlags cullMode;
        VkFrontFace frontFace;
        bool wireframe;
        
        // Stencil state
        bool stencilTestEnable;
        VkCompareOp stencilCompareOp;
        VkStencilOp stencilFailOp;
        VkStencilOp stencilPassOp;
        VkStencilOp stencilDepthFailOp;
        
        // Color write mask
        VkColorComponentFlags colorWriteMask;
        
        // Descriptor set layouts
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        
        // Push constant ranges
        std::vector<VkPushConstantRange> pushConstantRanges;
        
        // Dynamic states
        std::vector<VkDynamicState> dynamicStates;

    public:
        explicit VulkanPipelineBuilder(const VulkanDevice& device, VkRenderPass renderPass);

        // Shader configuration
        VulkanPipelineBuilder& addShader(const VulkanShader& shader);
        VulkanPipelineBuilder& addShaderStage(VkShaderStageFlagBits stage, VkShaderModule module, const char* entryPoint = "main");

        // Vertex input configuration
        VulkanPipelineBuilder& setVertexInput(const VkVertexInputBindingDescription& binding,
                                              const std::vector<VkVertexInputAttributeDescription>& attributes);
        
        template<typename VertexType>
        VulkanPipelineBuilder& setVertexInput() {
            return setVertexInput(VertexType::getBindingDescription(), VertexType::getAttributeDescriptions());
        }

        // Pipeline state configuration
        VulkanPipelineBuilder& setTopology(VkPrimitiveTopology topology);
        VulkanPipelineBuilder& setDepthTest(bool enable, bool writeEnable = true, VkCompareOp compareOp = VK_COMPARE_OP_LESS);
        VulkanPipelineBuilder& setBlending(bool enable, 
                                           VkBlendFactor srcColor = VK_BLEND_FACTOR_SRC_ALPHA, 
                                           VkBlendFactor dstColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                           VkBlendOp colorOp = VK_BLEND_OP_ADD,
                                           VkBlendFactor srcAlpha = VK_BLEND_FACTOR_ONE,
                                           VkBlendFactor dstAlpha = VK_BLEND_FACTOR_ZERO,
                                           VkBlendOp alphaOp = VK_BLEND_OP_ADD);
        VulkanPipelineBuilder& setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VulkanPipelineBuilder& setWireframe(bool enable);

        // Stencil configuration
        VulkanPipelineBuilder& setStencilTest(bool enable, 
                                              VkCompareOp compareOp = VK_COMPARE_OP_NOT_EQUAL,
                                              VkStencilOp failOp = VK_STENCIL_OP_KEEP,
                                              VkStencilOp passOp = VK_STENCIL_OP_KEEP,
                                              VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP);
        VulkanPipelineBuilder& setColorWriteMask(VkColorComponentFlags mask);

        // Descriptor set layouts
        VulkanPipelineBuilder& addDescriptorSetLayout(VkDescriptorSetLayout layout);

        // Push constants
        VulkanPipelineBuilder& addPushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);

        // Dynamic states
        VulkanPipelineBuilder& addDynamicState(VkDynamicState state);
        VulkanPipelineBuilder& setViewportDynamic();
        VulkanPipelineBuilder& setScissorDynamic();

        // Build the pipeline
        std::unique_ptr<VulkanPipeline> build();

    private:
        void setDefaults();
    };

    /**
     * Pipeline cache for managing and reusing pipelines
     */
    class VulkanPipelineCache {
    private:
        const VulkanDevice* device;
        VkPipelineCache pipelineCache;
        std::unordered_map<size_t, std::unique_ptr<VulkanPipeline>> cachedPipelines;

    public:
        explicit VulkanPipelineCache(const VulkanDevice& device);
        ~VulkanPipelineCache();

        // Non-copyable
        VulkanPipelineCache(const VulkanPipelineCache&) = delete;
        VulkanPipelineCache& operator=(const VulkanPipelineCache&) = delete;

        /**
         * Get or create pipeline with given configuration
         */
        VulkanPipeline* getOrCreatePipeline(size_t hash, const std::function<std::unique_ptr<VulkanPipeline>()>& creator);

        /**
         * Get Vulkan pipeline cache handle
         */
        VkPipelineCache getHandle() const { return pipelineCache; }

        /**
         * Save pipeline cache to file
         */
        void saveCacheToFile(const std::string& filename) const;

        /**
         * Load pipeline cache from file
         */
        void loadCacheFromFile(const std::string& filename);

    private:
        void cleanup();
    };

    /**
     * Pre-defined pipeline configurations for common use cases
     */
    namespace PipelinePresets {
        
        /**
         * Create color-only pipeline (solid colors, no textures)
         */
        std::unique_ptr<VulkanPipeline> createColorPipeline(const VulkanDevice& device, 
                                                            VkRenderPass renderPass,
                                                            VkDescriptorSetLayout uniformLayout);

        /**
         * Create material pipeline (textured surfaces)
         */
        std::unique_ptr<VulkanPipeline> createMaterialPipeline(const VulkanDevice& device, 
                                                               VkRenderPass renderPass,
                                                               VkDescriptorSetLayout uniformLayout,
                                                               VkDescriptorSetLayout textureLayout,
                                                               VkDescriptorSetLayout materialUniformLayout);

        /**
         * Create bloom filter pipeline (post-processing)
         */
        std::unique_ptr<VulkanPipeline> createBloomFilterPipeline(const VulkanDevice& device, 
                                                                  VkRenderPass renderPass,
                                                                  VkDescriptorSetLayout uniformLayout,
                                                                  VkDescriptorSetLayout textureLayout,
                                                                  VkDescriptorSetLayout bloomUniformLayout);

        /**
         * Create simple color pipeline using push constants (no descriptor sets needed)
         */
        std::unique_ptr<VulkanPipeline> createSimpleColorPipeline(const VulkanDevice& device, 
                                                                   VkRenderPass renderPass);

        /**
         * Create line pipeline using push constants (LINE_LIST topology)
         */
        std::unique_ptr<VulkanPipeline> createSimpleLinePipeline(const VulkanDevice& device,
                                                                  VkRenderPass renderPass);

        /**
         * Create line strip pipeline using push constants (LINE_STRIP topology)
         */
        std::unique_ptr<VulkanPipeline> createSimpleLineStripPipeline(const VulkanDevice& device,
                                                                       VkRenderPass renderPass);

        /**
         * Create point pipeline using push constants (POINT_LIST topology)
         */
        std::unique_ptr<VulkanPipeline> createSimplePointPipeline(const VulkanDevice& device,
                                                                    VkRenderPass renderPass);

        /**
         * Create a simple_color pipeline with arbitrary state parameters.
         * Used for on-demand pipeline creation based on current render state.
         */
        std::unique_ptr<VulkanPipeline> createSimpleColorPipelineWithState(
            const VulkanDevice& device,
            VkRenderPass renderPass,
            VkPrimitiveTopology topology,
            bool depthTest,
            bool depthWrite,
            BlendFunc blendFunc,
            bool wireframe,
            uint32_t stencilMode = 0);

        /**
         * Create a simple_texture pipeline with state parameters and a texture descriptor set layout.
         * Used for textured geometry with push constant MVP + material params.
         */
        std::unique_ptr<VulkanPipeline> createSimpleTexturePipelineWithState(
            const VulkanDevice& device,
            VkRenderPass renderPass,
            VkDescriptorSetLayout textureLayout,
            VkPrimitiveTopology topology,
            bool depthTest,
            bool depthWrite,
            BlendFunc blendFunc,
            bool wireframe,
            uint32_t stencilMode = 0);
    }

}

#endif
