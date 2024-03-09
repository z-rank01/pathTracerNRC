/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#pragma once

#include <cassert>
#include <iterator>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace nvvk {
//--------------------------------------------------------------------------------------------------
/** 
  # functions in nvvk

  - nvprintPipelineStats : prints stats of the pipeline using VK_KHR_pipeline_executable_properties (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
  - dumpPipelineStats    : dumps stats of the pipeline using VK_KHR_pipeline_executable_properties to a text file (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR)
  - dumpPipelineBinCodes : dumps shader binaries using VK_KHR_pipeline_executable_properties to multiple binary files (don't forget to enable extension and set VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)
*/
// nvprints stats to LOGLEVEL_STATS stream
void nvprintPipelineStats(VkDevice device, VkPipeline pipeline, const char* name, bool verbose = false);
// writes stats into single file
void dumpPipelineStats(VkDevice device, VkPipeline pipeline, const char* fileName);
// creates multiple files, one for each pipe executable and representation.
// The baseFilename will get appended along the lines of ".some details.bin"
void dumpPipelineInternals(VkDevice device, VkPipeline pipeline, const char* baseFileName);

//--------------------------------------------------------------------------------------------------
/** 
\struct nvvk::GraphicsPipelineState

Most graphic pipelines have similar states, therefore the helper `GraphicsPipelineStage` holds all the elements and 
initialize the structures with the proper default values, such as the primitive type, `PipelineColorBlendAttachmentState` 
with their mask, `DynamicState` for viewport and scissor, adjust depth test if enabled, line width to 1 pixel, for 
example. 

Example of usage :
\code{.cpp}
nvvk::GraphicsPipelineState pipelineState();
pipelineState.depthStencilState.setDepthTestEnable(true);
pipelineState.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineState.addBindingDescription({0, sizeof(Vertex)});
pipelineState.addAttributeDescriptions ({
    {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
    {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
    {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});
\endcode
*/


struct GraphicsPipelineState
{
  // Initialize the state to common values: triangle list topology, depth test enabled,
  // dynamic viewport and scissor, one render target, blending disabled
  GraphicsPipelineState()
  {
    rasterizationState.flags                   = {};
    rasterizationState.depthClampEnable        = {};
    rasterizationState.rasterizerDiscardEnable = {};
    setValue(rasterizationState.polygonMode, VK_POLYGON_MODE_FILL);
    setValue(rasterizationState.cullMode, VK_CULL_MODE_BACK_BIT);
    setValue(rasterizationState.frontFace, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    rasterizationState.depthBiasEnable         = {};
    rasterizationState.depthBiasConstantFactor = {};
    rasterizationState.depthBiasClamp          = {};
    rasterizationState.depthBiasSlopeFactor    = {};
    rasterizationState.lineWidth               = 1.f;

    inputAssemblyState.flags = {};
    setValue(inputAssemblyState.topology, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    inputAssemblyState.primitiveRestartEnable = {};


    colorBlendState.flags         = {};
    colorBlendState.logicOpEnable = {};
    setValue(colorBlendState.logicOp, VK_LOGIC_OP_CLEAR);
    colorBlendState.attachmentCount = {};
    colorBlendState.pAttachments    = {};
    for(int i = 0; i < 4; i++)
    {
      colorBlendState.blendConstants[i] = 0.f;
    }


    dynamicState.flags             = {};
    dynamicState.dynamicStateCount = {};
    dynamicState.pDynamicStates    = {};


    vertexInputState.flags                           = {};
    vertexInputState.vertexBindingDescriptionCount   = {};
    vertexInputState.pVertexBindingDescriptions      = {};
    vertexInputState.vertexAttributeDescriptionCount = {};
    vertexInputState.pVertexAttributeDescriptions    = {};


    viewportState.flags         = {};
    viewportState.viewportCount = {};
    viewportState.pViewports    = {};
    viewportState.scissorCount  = {};
    viewportState.pScissors     = {};


    depthStencilState.flags            = {};
    depthStencilState.depthTestEnable  = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    setValue(depthStencilState.depthCompareOp, VK_COMPARE_OP_LESS_OR_EQUAL);
    depthStencilState.depthBoundsTestEnable = {};
    depthStencilState.stencilTestEnable     = {};
    setValue(depthStencilState.front, VkStencilOpState());
    setValue(depthStencilState.back, VkStencilOpState());
    depthStencilState.minDepthBounds = {};
    depthStencilState.maxDepthBounds = {};

    setValue(multisampleState.rasterizationSamples, VK_SAMPLE_COUNT_1_BIT);
  }

  GraphicsPipelineState(const GraphicsPipelineState& src) = default;

  // Attach the pointer values of the structures to the internal arrays
  void update()
  {
    colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
    colorBlendState.pAttachments    = blendAttachmentStates.data();

    dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
    dynamicState.pDynamicStates    = dynamicStateEnables.data();

    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputState.pVertexBindingDescriptions      = bindingDescriptions.data();
    vertexInputState.pVertexAttributeDescriptions    = attributeDescriptions.data();

    if(viewports.empty())
    {
      viewportState.viewportCount = 1;
      viewportState.pViewports    = nullptr;
    }
    else
    {
      viewportState.viewportCount = (uint32_t)viewports.size();
      viewportState.pViewports    = viewports.data();
    }

    if(scissors.empty())
    {
      viewportState.scissorCount = 1;
      viewportState.pScissors    = nullptr;
    }
    else
    {
      viewportState.scissorCount = (uint32_t)scissors.size();
      viewportState.pScissors    = scissors.data();
    }
  }

  static inline VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(
      VkColorComponentFlags colorWriteMask_ = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      VkBool32      blendEnable_         = 0,
      VkBlendFactor srcColorBlendFactor_ = VK_BLEND_FACTOR_ZERO,
      VkBlendFactor dstColorBlendFactor_ = VK_BLEND_FACTOR_ZERO,
      VkBlendOp     colorBlendOp_        = VK_BLEND_OP_ADD,
      VkBlendFactor srcAlphaBlendFactor_ = VK_BLEND_FACTOR_ZERO,
      VkBlendFactor dstAlphaBlendFactor_ = VK_BLEND_FACTOR_ZERO,
      VkBlendOp     alphaBlendOp_        = VK_BLEND_OP_ADD)
  {
    VkPipelineColorBlendAttachmentState res;

    res.blendEnable         = blendEnable_;
    res.srcColorBlendFactor = srcColorBlendFactor_;
    res.dstColorBlendFactor = dstColorBlendFactor_;
    res.colorBlendOp        = colorBlendOp_;
    res.srcAlphaBlendFactor = srcAlphaBlendFactor_;
    res.dstAlphaBlendFactor = dstAlphaBlendFactor_;
    res.alphaBlendOp        = alphaBlendOp_;
    res.colorWriteMask      = colorWriteMask_;
    return res;
  }

  static inline VkVertexInputBindingDescription makeVertexInputBinding(uint32_t binding, uint32_t stride, VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX)
  {
    VkVertexInputBindingDescription vertexBinding;
    vertexBinding.binding   = binding;
    vertexBinding.inputRate = rate;
    vertexBinding.stride    = stride;
    return vertexBinding;
  }

  static inline VkVertexInputAttributeDescription makeVertexInputAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
  {
    VkVertexInputAttributeDescription attrib;
    attrib.binding  = binding;
    attrib.location = location;
    attrib.format   = format;
    attrib.offset   = offset;
    return attrib;
  }


  void clearBlendAttachmentStates() { blendAttachmentStates.clear(); }
  void setBlendAttachmentCount(uint32_t attachmentCount) { blendAttachmentStates.resize(attachmentCount); }

  void setBlendAttachmentState(uint32_t attachment, const VkPipelineColorBlendAttachmentState& blendState)
  {
    assert(attachment < blendAttachmentStates.size());
    if(attachment <= blendAttachmentStates.size())
    {
      blendAttachmentStates[attachment] = blendState;
    }
  }

  uint32_t addBlendAttachmentState(const VkPipelineColorBlendAttachmentState& blendState)
  {
    blendAttachmentStates.push_back(blendState);
    return (uint32_t)(blendAttachmentStates.size() - 1);
  }

  void clearDynamicStateEnables() { dynamicStateEnables.clear(); }
  void setDynamicStateEnablesCount(uint32_t dynamicStateCount) { dynamicStateEnables.resize(dynamicStateCount); }

  void setDynamicStateEnable(uint32_t state, VkDynamicState dynamicState)
  {
    assert(state < dynamicStateEnables.size());
    if(state <= dynamicStateEnables.size())
    {
      dynamicStateEnables[state] = dynamicState;
    }
  }

  uint32_t addDynamicStateEnable(VkDynamicState dynamicState)
  {
    dynamicStateEnables.push_back(dynamicState);
    return (uint32_t)(dynamicStateEnables.size() - 1);
  }


  void clearBindingDescriptions() { bindingDescriptions.clear(); }
  void setBindingDescriptionsCount(uint32_t bindingDescriptionCount)
  {
    bindingDescriptions.resize(bindingDescriptionCount);
  }
  void setBindingDescription(uint32_t binding, VkVertexInputBindingDescription bindingDescription)
  {
    assert(binding < bindingDescriptions.size());
    if(binding <= bindingDescriptions.size())
    {
      bindingDescriptions[binding] = bindingDescription;
    }
  }

  uint32_t addBindingDescription(const VkVertexInputBindingDescription& bindingDescription)
  {
    bindingDescriptions.push_back(bindingDescription);
    return (uint32_t)(bindingDescriptions.size() - 1);
  }

  void addBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& bindingDescriptions_)
  {
    bindingDescriptions.insert(bindingDescriptions.end(), bindingDescriptions_.begin(), bindingDescriptions_.end());
  }

  void clearAttributeDescriptions() { attributeDescriptions.clear(); }
  void setAttributeDescriptionsCount(uint32_t attributeDescriptionCount)
  {
    attributeDescriptions.resize(attributeDescriptionCount);
  }

  void setAttributeDescription(uint32_t attribute, const VkVertexInputAttributeDescription& attributeDescription)
  {
    assert(attribute < attributeDescriptions.size());
    if(attribute <= attributeDescriptions.size())
    {
      attributeDescriptions[attribute] = attributeDescription;
    }
  }


  uint32_t addAttributeDescription(const VkVertexInputAttributeDescription& attributeDescription)
  {
    attributeDescriptions.push_back(attributeDescription);
    return (uint32_t)(attributeDescriptions.size() - 1);
  }

  void addAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions_)
  {
    attributeDescriptions.insert(attributeDescriptions.end(), attributeDescriptions_.begin(), attributeDescriptions_.end());
  }


  void clearViewports() { viewports.clear(); }
  void setViewportsCount(uint32_t viewportCount) { viewports.resize(viewportCount); }
  void setViewport(uint32_t attribute, VkViewport viewport)
  {
    assert(attribute < viewports.size());
    if(attribute <= viewports.size())
    {
      viewports[attribute] = viewport;
    }
  }
  uint32_t addViewport(VkViewport viewport)
  {
    viewports.push_back(viewport);
    return (uint32_t)(viewports.size() - 1);
  }


  void clearScissors() { scissors.clear(); }
  void setScissorsCount(uint32_t scissorCount) { scissors.resize(scissorCount); }
  void setScissor(uint32_t attribute, VkRect2D scissor)
  {
    assert(attribute < scissors.size());
    if(attribute <= scissors.size())
    {
      scissors[attribute] = scissor;
    }
  }
  uint32_t addScissor(VkRect2D scissor)
  {
    scissors.push_back(scissor);
    return (uint32_t)(scissors.size() - 1);
  }


  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  VkPipelineRasterizationStateCreateInfo rasterizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  VkPipelineMultisampleStateCreateInfo   multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  VkPipelineDepthStencilStateCreateInfo  depthStencilState{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  VkPipelineViewportStateCreateInfo      viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  VkPipelineDynamicStateCreateInfo       dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  VkPipelineColorBlendStateCreateInfo    colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  VkPipelineVertexInputStateCreateInfo   vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

protected:
  std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{makePipelineColorBlendAttachmentState()};
  std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  std::vector<VkVertexInputBindingDescription>   bindingDescriptions;
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

  std::vector<VkViewport> viewports;
  std::vector<VkRect2D>   scissors;


  // Helper to set objects for either C and C++
  template <class T, class U>
  void setValue(T& target, const U& val)
  {
    target = (T)(val);
  }
};


//--------------------------------------------------------------------------------------------------
/** 
\struct nvvk::GraphicsPipelineGenerator

The graphics pipeline generator takes a GraphicsPipelineState object and pipeline-specific information such as 
the render pass and pipeline layout to generate the final pipeline. 

Example of usage :
\code{.cpp}
nvvk::GraphicsPipelineState pipelineState();
...
nvvk::GraphicsPipelineGenerator pipelineGenerator(m_device, m_pipelineLayout, m_renderPass, pipelineState);
pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
\endcode
*/

struct GraphicsPipelineGenerator
{
public:
  GraphicsPipelineGenerator(GraphicsPipelineState& pipelineState_)
      : pipelineState(pipelineState_)
  {
    init();
  }

  GraphicsPipelineGenerator(const GraphicsPipelineGenerator& src)
      : createInfo(src.createInfo)
      , device(src.device)
      , pipelineCache(src.pipelineCache)
      , pipelineState(src.pipelineState)
  {
    init();
  }

  GraphicsPipelineGenerator(VkDevice device_, const VkPipelineLayout& layout, const VkRenderPass& renderPass, GraphicsPipelineState& pipelineState_)
      : device(device_)
      , pipelineState(pipelineState_)
  {
    createInfo.layout     = layout;
    createInfo.renderPass = renderPass;
    init();
  }

  // For VK_KHR_dynamic_rendering
  using PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo;

  GraphicsPipelineGenerator(VkDevice                           device_,
                            const VkPipelineLayout&            layout,
                            const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
                            GraphicsPipelineState&             pipelineState_)
      : device(device_)
      , pipelineState(pipelineState_)
  {
    createInfo.layout = layout;
    setPipelineRenderingCreateInfo(pipelineRenderingCreateInfo);
    init();
  }

  const GraphicsPipelineGenerator& operator=(const GraphicsPipelineGenerator& src)
  {
    device        = src.device;
    pipelineState = src.pipelineState;
    createInfo    = src.createInfo;
    pipelineCache = src.pipelineCache;

    init();
    return *this;
  }

  void setDevice(VkDevice device_) { device = device_; }

  void setRenderPass(VkRenderPass renderPass)
  {
    createInfo.renderPass = renderPass;
    createInfo.pNext      = nullptr;
  }

  void setPipelineRenderingCreateInfo(const PipelineRenderingCreateInfo& pipelineRenderingCreateInfo)
  {
    // Deep copy
    assert(pipelineRenderingCreateInfo.pNext == nullptr);  // Update deep copy if needed.
    dynamicRenderingInfo = pipelineRenderingCreateInfo;
    if(dynamicRenderingInfo.colorAttachmentCount != 0)
    {
      dynamicRenderingColorFormats.assign(dynamicRenderingInfo.pColorAttachmentFormats,
                                          dynamicRenderingInfo.pColorAttachmentFormats + dynamicRenderingInfo.colorAttachmentCount);
      dynamicRenderingInfo.pColorAttachmentFormats = dynamicRenderingColorFormats.data();
    }

    // Set VkGraphicsPipelineCreateInfo::pNext to point to deep copy of extension struct.
    // NB: Will have to change if more than 1 extension struct needs to be supported.
    createInfo.pNext = &dynamicRenderingInfo;
  }

  void setLayout(VkPipelineLayout layout) { createInfo.layout = layout; }

  ~GraphicsPipelineGenerator() { destroyShaderModules(); }

  VkPipelineShaderStageCreateInfo& addShader(const std::string& code, VkShaderStageFlagBits stage, const char* entryPoint = "main")
  {
    std::vector<char> v;
    std::copy(code.begin(), code.end(), std::back_inserter(v));
    return addShader(v, stage, entryPoint);
  }

  template <typename T>
  VkPipelineShaderStageCreateInfo& addShader(const std::vector<T>& code, VkShaderStageFlagBits stage, const char* entryPoint = "main")

  {
    VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = sizeof(T) * code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    temporaryModules.push_back(shaderModule);

    return addShader(shaderModule, stage, entryPoint);
  }
  VkPipelineShaderStageCreateInfo& addShader(VkShaderModule shaderModule, VkShaderStageFlagBits stage, const char* entryPoint = "main")
  {
    VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStage.stage  = (VkShaderStageFlagBits)stage;
    shaderStage.module = shaderModule;
    shaderStage.pName  = entryPoint;

    shaderStages.push_back(shaderStage);
    return shaderStages.back();
  }

  void clearShaders()
  {
    shaderStages.clear();
    destroyShaderModules();
  }

  VkShaderModule getShaderModule(size_t index) const
  {
    if(index < shaderStages.size())
      return shaderStages[index].module;
    return VK_NULL_HANDLE;
  }

  VkPipeline createPipeline(const VkPipelineCache& cache)
  {
    update();
    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, cache, 1, (VkGraphicsPipelineCreateInfo*)&createInfo, nullptr, &pipeline);
    return pipeline;
  }

  VkPipeline createPipeline() { return createPipeline(pipelineCache); }

  void destroyShaderModules()
  {
    for(const auto& shaderModule : temporaryModules)
    {
      vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    temporaryModules.clear();
  }
  void update()
  {
    createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    createInfo.pStages    = shaderStages.data();
    pipelineState.update();
  }

  VkGraphicsPipelineCreateInfo createInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

private:
  VkDevice        device;
  VkPipelineCache pipelineCache{};

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  std::vector<VkShaderModule>                  temporaryModules;
  std::vector<VkFormat>                        dynamicRenderingColorFormats;
  GraphicsPipelineState&                       pipelineState;
  PipelineRenderingCreateInfo                  dynamicRenderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};


  void init()
  {
    createInfo.pRasterizationState = &pipelineState.rasterizationState;
    createInfo.pInputAssemblyState = &pipelineState.inputAssemblyState;
    createInfo.pColorBlendState    = &pipelineState.colorBlendState;
    createInfo.pMultisampleState   = &pipelineState.multisampleState;
    createInfo.pViewportState      = &pipelineState.viewportState;
    createInfo.pDepthStencilState  = &pipelineState.depthStencilState;
    createInfo.pDynamicState       = &pipelineState.dynamicState;
    createInfo.pVertexInputState   = &pipelineState.vertexInputState;
  }

  // Helper to set objects for either C and C++
  template <class T, class U>
  void setValue(T& target, const U& val)
  {
    target = (T)(val);
  }
};


//--------------------------------------------------------------------------------------------------
/** 
\class nvvk::GraphicsPipelineGeneratorCombined

In some cases the application may have each state associated to a single pipeline. For convenience, 
nvvk::GraphicsPipelineGeneratorCombined combines both the state and generator into a single object.

Example of usage :
\code{.cpp}
nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(m_device, m_pipelineLayout, m_renderPass);
pipelineGenerator.depthStencilState.setDepthTestEnable(true);
pipelineGenerator.rasterizationState.setCullMode(vk::CullModeFlagBits::eNone);
pipelineGenerator.addBindingDescription({0, sizeof(Vertex)});
pipelineGenerator.addAttributeDescriptions ({
    {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
    {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
    {2, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, col))}});

pipelineGenerator.addShader(readFile("spv/vert_shader.vert.spv"), VkShaderStageFlagBits::eVertex);
pipelineGenerator.addShader(readFile("spv/frag_shader.frag.spv"), VkShaderStageFlagBits::eFragment);

m_pipeline = pipelineGenerator.createPipeline();
\endcode
*/


struct GraphicsPipelineGeneratorCombined : public GraphicsPipelineState, public GraphicsPipelineGenerator
{
  GraphicsPipelineGeneratorCombined(VkDevice device_, const VkPipelineLayout& layout, const VkRenderPass& renderPass)
      : GraphicsPipelineState()
      , GraphicsPipelineGenerator(device_, layout, renderPass, *this)
  {
  }
};


//--------------------------------------------------------------------------------------------------
/** 
\struct nvvk::GraphicShaderObjectPipeline

This is a helper to set the dynamic graphics pipeline state for shader object
 - Set the pipeline state as you would do for a regular pipeline
 - Call cmdSetPipelineState to set the pipeline state in the command buffer

Example of usage :
\code{.cpp}
  // Member of the class
  nvvk::GraphicShaderObjectPipeline m_shaderObjPipeline;


  // Creation of the dynamic graphic pipeline
  m_shaderObjPipeline.rasterizationState.cullMode = VK_CULL_MODE_NONE;
  m_shaderObjPipeline.addBindingDescriptions({{0, sizeof(nvh::PrimitiveVertex)}});
  m_shaderObjPipeline.addAttributeDescriptions({
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(nvh::PrimitiveVertex, p))},  // Position
    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(nvh::PrimitiveVertex, n))},  // Normal
    });
  m_shaderObjPipeline.update();

  // In the drawing
  m_shaderObjPipeline.setViewportScissor(m_app->getViewportSize());
  m_shaderObjPipeline.cmdSetPipelineState(cmd);

\endcode
*/

struct GraphicShaderObjectPipeline : GraphicsPipelineState
{
  VkSampleMask                                       sampleMask{~0U};
  std::vector<VkVertexInputBindingDescription2EXT>   vertexBindingDescriptions2;
  std::vector<VkColorBlendEquationEXT>               colorBlendEquationState;
  std::vector<VkBool32>                              colorBlendEnables;
  std::vector<VkBool32>                              colorWriteMasks;
  std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributeDescriptions2;

  GraphicShaderObjectPipeline()
  {
    viewports.resize(1);  // There should be at least one viewport
    scissors.resize(1);   // 
  }

  // Set the viewport and scissor to the full extent
  void setViewportScissor(const VkExtent2D& extent)
  {
    viewports[0].x        = 0;
    viewports[0].y        = 0;
    viewports[0].width    = float(extent.width);
    viewports[0].height   = float(extent.height);
    viewports[0].minDepth = 0;
    viewports[0].maxDepth = 1;

    scissors[0].offset = {0, 0};
    scissors[0].extent = extent;
  }

  // Update the internal state
  void update()
  {
    GraphicsPipelineState::update();
    multisampleState.pSampleMask = &sampleMask;

    vertexBindingDescriptions2.resize(vertexInputState.vertexBindingDescriptionCount);
    for(uint32_t i = 0; i < vertexInputState.vertexBindingDescriptionCount; i++)
    {
      vertexBindingDescriptions2[i].sType     = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
      vertexBindingDescriptions2[i].binding   = vertexInputState.pVertexBindingDescriptions[i].binding;
      vertexBindingDescriptions2[i].inputRate = vertexInputState.pVertexBindingDescriptions[i].inputRate;
      vertexBindingDescriptions2[i].stride    = vertexInputState.pVertexBindingDescriptions[i].stride;
      vertexBindingDescriptions2[i].divisor   = 1;
    }

    vertexAttributeDescriptions2.resize(vertexInputState.vertexAttributeDescriptionCount);
    for(uint32_t i = 0; i < vertexInputState.vertexAttributeDescriptionCount; i++)
    {
      vertexAttributeDescriptions2[i].sType    = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
      vertexAttributeDescriptions2[i].binding  = vertexInputState.pVertexAttributeDescriptions[i].binding;
      vertexAttributeDescriptions2[i].format   = vertexInputState.pVertexAttributeDescriptions[i].format;
      vertexAttributeDescriptions2[i].location = vertexInputState.pVertexAttributeDescriptions[i].location;
      vertexAttributeDescriptions2[i].offset   = vertexInputState.pVertexAttributeDescriptions[i].offset;
    }

    colorBlendEquationState.resize(colorBlendState.attachmentCount);
    colorBlendEnables.resize(colorBlendState.attachmentCount);
    colorWriteMasks.resize(colorBlendState.attachmentCount);
    for(uint32_t i = 0; i < colorBlendState.attachmentCount; i++)
    {
      colorBlendEquationState[i].srcColorBlendFactor = colorBlendState.pAttachments[i].srcColorBlendFactor;
      colorBlendEquationState[i].dstColorBlendFactor = colorBlendState.pAttachments[i].dstColorBlendFactor;
      colorBlendEquationState[i].colorBlendOp        = colorBlendState.pAttachments[i].colorBlendOp;
      colorBlendEquationState[i].srcAlphaBlendFactor = colorBlendState.pAttachments[i].srcAlphaBlendFactor;
      colorBlendEquationState[i].dstAlphaBlendFactor = colorBlendState.pAttachments[i].dstAlphaBlendFactor;
      colorBlendEquationState[i].alphaBlendOp        = colorBlendState.pAttachments[i].alphaBlendOp;
      colorBlendEnables[i]                           = colorBlendState.pAttachments[i].blendEnable;
      colorWriteMasks[i]                             = colorBlendState.pAttachments[i].colorWriteMask;
    }
  }

  // Set the pipeline state in the command buffer
  void cmdSetPipelineState(VkCommandBuffer cmd)
  {
    vkCmdSetViewportWithCount(cmd, viewportState.viewportCount, viewportState.pViewports);
    vkCmdSetScissorWithCount(cmd, viewportState.scissorCount, viewportState.pScissors);

    vkCmdSetLineWidth(cmd, rasterizationState.lineWidth);
    vkCmdSetDepthBias(cmd, rasterizationState.depthBiasConstantFactor, rasterizationState.depthBiasClamp,
                      rasterizationState.depthBiasSlopeFactor);
    vkCmdSetCullMode(cmd, rasterizationState.cullMode);
    vkCmdSetFrontFace(cmd, rasterizationState.frontFace);
    vkCmdSetDepthBiasEnable(cmd, rasterizationState.depthBiasEnable);
    vkCmdSetRasterizerDiscardEnable(cmd, rasterizationState.rasterizerDiscardEnable);
    vkCmdSetDepthClampEnableEXT(cmd, rasterizationState.depthClampEnable);
    vkCmdSetPolygonModeEXT(cmd, rasterizationState.polygonMode);

    vkCmdSetBlendConstants(cmd, colorBlendState.blendConstants);

    vkCmdSetDepthBounds(cmd, depthStencilState.minDepthBounds, depthStencilState.maxDepthBounds);
    vkCmdSetDepthBoundsTestEnable(cmd, depthStencilState.depthBoundsTestEnable);
    vkCmdSetDepthCompareOp(cmd, depthStencilState.depthCompareOp);
    vkCmdSetDepthTestEnable(cmd, depthStencilState.depthTestEnable);
    vkCmdSetDepthWriteEnable(cmd, depthStencilState.depthWriteEnable);
    vkCmdSetStencilTestEnable(cmd, depthStencilState.stencilTestEnable);

    vkCmdSetPrimitiveRestartEnable(cmd, inputAssemblyState.primitiveRestartEnable);
    vkCmdSetPrimitiveTopology(cmd, inputAssemblyState.topology);

    vkCmdSetRasterizationSamplesEXT(cmd, multisampleState.rasterizationSamples);
    vkCmdSetSampleMaskEXT(cmd, multisampleState.rasterizationSamples, multisampleState.pSampleMask);
    vkCmdSetAlphaToCoverageEnableEXT(cmd, multisampleState.alphaToCoverageEnable);
    vkCmdSetAlphaToOneEnableEXT(cmd, multisampleState.alphaToOneEnable);

    vkCmdSetVertexInputEXT(cmd, vertexInputState.vertexBindingDescriptionCount, vertexBindingDescriptions2.data(),
                           vertexInputState.vertexAttributeDescriptionCount, vertexAttributeDescriptions2.data());

    vkCmdSetColorBlendEquationEXT(cmd, 0, colorBlendState.attachmentCount, colorBlendEquationState.data());
    vkCmdSetColorBlendEnableEXT(cmd, 0, colorBlendState.attachmentCount, colorBlendEnables.data());
    vkCmdSetColorWriteMaskEXT(cmd, 0, colorBlendState.attachmentCount, colorWriteMasks.data());
    vkCmdSetLogicOpEnableEXT(cmd, colorBlendState.logicOpEnable);
  }
};


}  // namespace nvvk
