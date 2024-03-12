#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <cassert>
#include <array>
#include <utility.h>

//#include <nvh/fileoperations.hpp>           // For nvh::loadfiles
//#include <nvvk/descriptorsets_vk.hpp>		// For nvvk::DescriptorSetContainer
//#include <nvvk/context_vk.hpp>
//#include <nvvk/shaders_vk.hpp>				// For nvvk::createShaderModule
//#include <nvvk/structs_vk.hpp>				// For nvvk::make
//#include <nvvk/resourceallocator_vk.hpp>	// For NVVK memory allocators
//#include <nvvk/error_vk.hpp>                // For NVVK_CHECK

int main(int argc, const char** argv)
{
	// ---------
	// Constants
	// ---------
	static const uint64_t img_width = 800;
	static const uint64_t img_height = 600;
	static const uint32_t workgroup_width = 16;
	static const uint32_t workgroup_height = 8;
	// possible paths of shader and other files
	const std::string exePath(argv[0], std::string(argv[0]).find_last_of("/\\") + 1);
	std::vector<std::string> searchPaths = {exePath + PROJECT_RELDIRECTORY,
											exePath + PROJECT_RELDIRECTORY "..",
											exePath + PROJECT_RELDIRECTORY "../..",
											exePath + PROJECT_NAME };

	// load .obj model
	tinyobj::ObjReader       reader;  // Used to read an OBJ file
	reader.ParseFromFile(nvh::findFile("scenes/CornellBox-Original-Merged.obj", searchPaths));
	assert(reader.Valid());  // Make sure tinyobj was able to parse this file
	// vertices
	const std::vector<tinyobj::real_t> cornellBox_vertices = reader.GetAttrib().GetVertices();
	// indices
	const std::vector<tinyobj::shape_t> cornellBox_shapes = reader.GetShapes();
	assert(cornellBox_shapes.size() == 1);   // ensure that shapes of obj is only 1 (scene)
	const tinyobj::shape_t& cornellBox_shape = cornellBox_shapes[0];
	std::vector<uint32_t> cornellBox_indices;
	cornellBox_indices.reserve(cornellBox_shape.mesh.indices.size());
	for (const tinyobj::index_t& index : cornellBox_shape.mesh.indices)
	{
		cornellBox_indices.push_back(index.vertex_index);
	}


	// ---------------------
	// Create Vulkan context
	// ---------------------
	nvvk::ContextCreateInfo ctxInfo;	 // consisting of an instance, device, physical device, and queues.
	// One can modify this to load different extensions or pick the Vulkan core version
	// Vulkan API
	ctxInfo.apiMajor = 1;                // major: actual version we will use
	ctxInfo.apiMinor = 3;                // minor: least version we require
	// Ray query extension
	ctxInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);			// extension 1: deferred operation (dependencies for acceleration sturcture and ray query) 
	auto asFeature = nvvk::make<VkPhysicalDeviceAccelerationStructureFeaturesKHR>();	// extension 2: acceleration structure (dependencies for ray query and for accelerate ray interaction) 
	ctxInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &asFeature);
	auto rqFeature = nvvk::make<VkPhysicalDeviceRayQueryFeaturesKHR>();					// extension 3: ray query (for ray tracing) 
	ctxInfo.addDeviceExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, false, &rqFeature);
	//shader debugPrintf extension (just for shader debug)
	//ctxInfo.addDeviceExtension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
	//VkValidationFeaturesEXT      validationInfo = nvvk::make<VkValidationFeaturesEXT>();
	//VkValidationFeatureEnableEXT validationFeatureToEnable = VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT;
	//validationInfo.enabledValidationFeatureCount = 1;
	//validationInfo.pEnabledValidationFeatures = &validationFeatureToEnable;
	//ctxInfo.instanceCreateInfoExt = &validationInfo;
	//#ifdef _WIN32
	//	_putenv_s("DEBUG_PRINTF_TO_STDOUT", "1");
	//#else   // If not _WIN32
	//	static char putenvString[] = "DEBUG_PRINTF_TO_STDOUT=1";
	//	putenv(putenvString);
	//#endif  // _WIN32


	// -------------------------
	// Initialize Vulkan context
	// -------------------------
	nvvk::Context context;               // Encapsulates device state in a single object
	context.init(ctxInfo);
	assert(asFeature.accelerationStructure == VK_TRUE && rqFeature.rayQuery == VK_TRUE); // Device must support acceleration structures and ray queries.


	// ------------------------
	// Initialize the allocator
	// ------------------------
	nvvk::ResourceAllocatorDedicated allocator;
	allocator.init(context.m_device, context.m_physicalDevice);
	

	// -------------------
	// Create Command Pool
	// -------------------
	auto cmdPoolCreateInfo = nvvk::make<VkCommandPoolCreateInfo>();
	cmdPoolCreateInfo.queueFamilyIndex = context.m_queueGCT;
	VkCommandPool cmdPool;
	NVVK_CHECK(vkCreateCommandPool(context.m_device, &cmdPoolCreateInfo, nullptr, &cmdPool));   // nullptr means using default Vulkan memory allocator.


	// ----------------
	// Create Resources
	// ----------------
	VkDeviceSize bufferSizeBytes = img_width * img_height * 3 * sizeof(float);  // 3-channel of render output image in size of (width, height)
	auto bufferCreateInfo = nvvk::make<VkBufferCreateInfo>();
	bufferCreateInfo.size = bufferSizeBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	nvvk::Buffer stgBuffer = allocator.createBuffer(bufferCreateInfo,   // this buffer will be used for writting and storing data.
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT								// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT means that the CPU can read this buffer's memory.
		| VK_MEMORY_PROPERTY_HOST_CACHED_BIT							// VK_MEMORY_PROPERTY_HOST_CACHED_BIT means that the CPU caches this memory.
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);						// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT means that the CPU side of cache management
																		// is handled automatically, with potentially slower reads/writes.
	
	// Create two local-host buffer for vertex and index
	// this reuiqre two staging buffer to transfer from CPU to GPU
	VkCommandBuffer storage2LocalCmdBuffer = NRC::beginSingleTimeCommandRecord(context.m_device, cmdPool);
	
	const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
											   | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	const VkMemoryPropertyFlags memPropFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	const VkBufferUsageFlags stagingBufferUsageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	const VkMemoryPropertyFlags stagingMemPropFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VkBuffer tempStagingBuffer2vertex;
	VkBuffer tempStagingBuffer2index;
	VkBuffer vertexBuffer; 
	VkBuffer indexBuffer;
	VkDeviceSize vertexBufferSizeBytes = sizeof(float) * cornellBox_vertices.size();
	VkDeviceSize indexBufferSizeBytes = sizeof(uint32_t) * cornellBox_indices.size();

	VkDeviceMemory tempStagingBufferMemory2vertex;
	VkDeviceMemory tempStagingBufferMemory2index;
	VkDeviceMemory vertexBufferMemory;
	VkDeviceMemory indexBufferMemory;

	// create vertex buffer and transfer from staging buffer to device-local
	NRC::createBuffer(context, storage2LocalCmdBuffer, 
					  vertexBufferSizeBytes,
					  &tempStagingBuffer2vertex, stagingBufferUsageFlags, 
					  &tempStagingBufferMemory2vertex, stagingMemPropFlags);
	void* vertexData;
	vkMapMemory(context.m_device, tempStagingBufferMemory2vertex, 0, vertexBufferSizeBytes, 0, &vertexData);
	memcpy(vertexData, &cornellBox_vertices, (size_t)vertexBufferSizeBytes);
	vkUnmapMemory(context.m_device, tempStagingBufferMemory2vertex);
	NRC::createBuffer(context, storage2LocalCmdBuffer,
					  vertexBufferSizeBytes,
					  &vertexBuffer, bufferUsageFlags, 
					  &vertexBufferMemory, memPropFlags);
	NRC::copyBuffer(storage2LocalCmdBuffer, tempStagingBuffer2vertex, vertexBuffer, vertexBufferSizeBytes);

	// create index buffer and transfer from staging buffer to device-local
	NRC::createBuffer(context, storage2LocalCmdBuffer,
					  indexBufferSizeBytes,
					  &tempStagingBuffer2index, stagingBufferUsageFlags,
					  &tempStagingBufferMemory2index, stagingMemPropFlags);
	void* indexData;
	vkMapMemory(context.m_device, tempStagingBufferMemory2index, 0, indexBufferSizeBytes, 0, &indexData);
	memcpy(indexData, &cornellBox_indices, (size_t)indexBufferSizeBytes);
	vkUnmapMemory(context.m_device, tempStagingBufferMemory2index);
	NRC::createBuffer(context, storage2LocalCmdBuffer, 
					  indexBufferSizeBytes,
					  &indexBuffer, bufferUsageFlags, 
					  &indexBufferMemory, memPropFlags);
	NRC::copyBuffer(storage2LocalCmdBuffer, tempStagingBuffer2index, indexBuffer, indexBufferSizeBytes);

	// end command
	NRC::endSubmitSingleTimeCommandRecord(context.m_device, context.m_queueGCT, cmdPool, storage2LocalCmdBuffer);
	
	// clean up
	vkDestroyBuffer(context.m_device, tempStagingBuffer2vertex, nullptr);
	vkDestroyBuffer(context.m_device, tempStagingBuffer2index, nullptr);
	vkFreeMemory(context.m_device, tempStagingBufferMemory2vertex, nullptr);
	vkFreeMemory(context.m_device, tempStagingBufferMemory2index, nullptr);

	/*nvvk::Buffer vertexBuffer = allocator.createBuffer(storage2LocalCmdBuffer, cornellBox_vertices, bufferUsageFlags);
	nvvk::Buffer indexBuffer = allocator.createBuffer(storage2LocalCmdBuffer, cornellBox_indices, bufferUsageFlags);
	NRC::endSubmitSingleTimeCommandRecord(context.m_device, context.m_queueGCT, cmdPool, storage2LocalCmdBuffer);
	allocator.finalizeAndReleaseStaging();*/


	// --------------------
	// Create Shader Module
	// --------------------
	VkShaderModule rayTracerShaderModule = nvvk::createShaderModule(context.m_device, 
																	nvh::loadFile("shaders/raytracer.comp.glsl.spv", true, searchPaths));
	
	// -----------------------------------------
	// Create Descriptor Set bindings and layout
	// -----------------------------------------
	VkDescriptorSetLayoutBinding storageBufferBinding0{};
	storageBufferBinding0.binding			= 0;
	storageBufferBinding0.descriptorCount	= 1;
	storageBufferBinding0.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageBufferBinding0.stageFlags		= VK_SHADER_STAGE_COMPUTE_BIT;
	

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreatInfo = nvvk::make<VkDescriptorSetLayoutCreateInfo>();
	descriptorSetLayoutCreatInfo.bindingCount	= 1;
	descriptorSetLayoutCreatInfo.pBindings		= &storageBufferBinding0;
	VkDescriptorSetLayout descriptorSetLayout;
	NVVK_CHECK(vkCreateDescriptorSetLayout(context.m_device, &descriptorSetLayoutCreatInfo, nullptr, &descriptorSetLayout));


	// ----------------------------------------
	// Create Descriptor Pool and allocate Sets
	// ----------------------------------------
	VkDescriptorPoolSize descriptorPoolSize = nvvk::make<VkDescriptorPoolSize>();
	descriptorPoolSize.descriptorCount	= 1;
	descriptorPoolSize.type				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = nvvk::make<VkDescriptorPoolCreateInfo>();
	descriptorPoolCreateInfo.maxSets		= 1;
	descriptorPoolCreateInfo.poolSizeCount	= 1;
	descriptorPoolCreateInfo.pPoolSizes		= &descriptorPoolSize;
	VkDescriptorPool descriptorPool;
	NVVK_CHECK(vkCreateDescriptorPool(context.m_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = nvvk::make<VkDescriptorSetAllocateInfo>();
	descriptorSetAllocateInfo.descriptorPool		= descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount	= 1;
	descriptorSetAllocateInfo.pSetLayouts			= &descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorsets;
	descriptorsets.resize(1);
	NVVK_CHECK(vkAllocateDescriptorSets(context.m_device, &descriptorSetAllocateInfo, descriptorsets.data()));


	// --------------------------------
	// Write and update descriptor sets
	// --------------------------------
	VkDescriptorBufferInfo descriptorBufferInfo{};
	descriptorBufferInfo.buffer		= stgBuffer.buffer;
	descriptorBufferInfo.offset		= 0;
	descriptorBufferInfo.range		= bufferSizeBytes;

	VkWriteDescriptorSet writeDescriptorSet = nvvk::make<VkWriteDescriptorSet>();
	writeDescriptorSet.descriptorCount	= 1;
	writeDescriptorSet.descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSet.dstArrayElement	= 0;
	writeDescriptorSet.dstBinding		= 0;
	writeDescriptorSet.dstSet			= descriptorsets[0];
	writeDescriptorSet.pBufferInfo		= &descriptorBufferInfo;
	vkUpdateDescriptorSets(context.m_device, 1, &writeDescriptorSet, 0, nullptr);


	// ---------------
	// Create Pipeline
	// ---------------
	
	// shader stage in pipeline
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
	shaderStageCreateInfo.sType		= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage		= VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module	= rayTracerShaderModule;
	shaderStageCreateInfo.pName		= "main";                      // this define the entry point used in shaders.
	
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = nvvk::make<VkPipelineLayoutCreateInfo>();
	pipelineLayoutCreateInfo.setLayoutCount			= 1;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts			= &descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	NVVK_CHECK(vkCreatePipelineLayout(context.m_device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));

	VkComputePipelineCreateInfo computePipelineCreateInfo = nvvk::make<VkComputePipelineCreateInfo>();
	computePipelineCreateInfo.layout	= pipelineLayout;
	computePipelineCreateInfo.stage		= shaderStageCreateInfo;
	VkPipeline computePipeline;
	NVVK_CHECK(vkCreateComputePipelines(context.m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, VK_NULL_HANDLE, &computePipeline));


	// -----------------------
	// Record Dispatch Command
	// -----------------------
	VkCommandBuffer cmdBuffer = NRC::beginSingleTimeCommandRecord(context.m_device, cmdPool);

	// bind compute pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
							descriptorsets.data(), 0, nullptr);
	// use compute shader
	vkCmdDispatch(cmdBuffer, (uint32_t(img_width) + workgroup_width - 1) / workgroup_width,
							 (uint32_t(img_height) + workgroup_height - 1) / workgroup_height, 
							 1);

	// add barrier
	// -------
	auto mBarrier = nvvk::make<VkMemoryBarrier>();
	mBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	mBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(cmdBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,	// src stage: wait for compute shader stage to finish
		VK_PIPELINE_STAGE_HOST_BIT,				// dst stage: make cache available and visible for CPU
		0,										// dependency flags
		1, &mBarrier,							// memory barrier
		0, nullptr,								// buffer memory barrier
		0, nullptr);							// image memory barrier

	// end record and submit
	NRC::endSubmitSingleTimeCommandRecord(context.m_device, context.m_queueGCT, cmdPool, cmdBuffer);


	// -----------------------------
	// Data-Buffer mapping and usage
	// -----------------------------
	void* data = allocator.map(stgBuffer);
	// float* fData = reinterpret_cast<float*>(data);
	// printf("First three element in GPU buffer: %f, %f, %f\n", fData[0], fData[1], fData[2]);
	stbi_write_hdr("../../outputs/pixelColor.hdr", img_width, img_height, 3, reinterpret_cast<float*>(data));
	allocator.unmap(stgBuffer);


	// --------
	// Clean up
	// --------
	descriptorsets.clear();
	vkDestroyBuffer(context.m_device, vertexBuffer, nullptr);
	vkDestroyBuffer(context.m_device, indexBuffer, nullptr);
	vkFreeMemory(context.m_device, vertexBufferMemory, nullptr);
	vkFreeMemory(context.m_device, indexBufferMemory, nullptr);
	vkDestroyDescriptorPool(context.m_device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(context.m_device, descriptorSetLayout, nullptr);
	vkDestroyShaderModule(context.m_device, rayTracerShaderModule, nullptr);
	vkDestroyPipelineLayout(context.m_device, pipelineLayout, nullptr);
	vkDestroyPipeline(context.m_device, computePipeline, nullptr);
	// vkFreeCommandBuffers(context.m_device, cmdPool, 1, &cmdBuffer);
	vkDestroyCommandPool(context.m_device, cmdPool, nullptr);
	//allocator.destroy(vertexBuffer);
	//allocator.destroy(indexBuffer);
	allocator.destroy(stgBuffer);
	allocator.deinit();
	context.deinit();
}