# pragma once

#include <nvh/fileoperations.hpp>           // For nvh::loadfiles
#include <nvvk/descriptorsets_vk.hpp>		// For nvvk::DescriptorSetContainer
#include <nvvk/context_vk.hpp>
#include <nvvk/shaders_vk.hpp>				// For nvvk::createShaderModule
#include <nvvk/structs_vk.hpp>				// For nvvk::make
#include <nvvk/resourceallocator_vk.hpp>	// For NVVK memory allocators
#include <nvvk/error_vk.hpp>                // For NVVK_CHECK

namespace NRC
{ 
	static VkCommandBuffer beginSingleTimeCommandRecord(VkDevice device, VkCommandPool cmdPool)
	{
		// -----------------------
		// Allocate Command Buffer
		// -----------------------
		auto cmdBufferAllocateInfo = nvvk::make<VkCommandBufferAllocateInfo>();
		cmdBufferAllocateInfo.commandBufferCount = 1;
		cmdBufferAllocateInfo.commandPool = cmdPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VkCommandBuffer cmdBuffer;
		NVVK_CHECK(vkAllocateCommandBuffers(device, &cmdBufferAllocateInfo, &cmdBuffer));


		// --------------
		// Record Command
		// --------------

		// begin command record
		auto cmdBufferBeginInfo = nvvk::make<VkCommandBufferBeginInfo>();
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		NVVK_CHECK(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
		return cmdBuffer;
	}

	static void endSubmitSingleTimeCommandRecord(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkCommandBuffer& cmdBuffer)
	{
		NVVK_CHECK(vkEndCommandBuffer(cmdBuffer));

		// --------------
		// Submit Command
		// --------------
		auto submitInfo = nvvk::make<VkSubmitInfo>();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		NVVK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// ---------------
		// Queue Wait Idle
		// ---------------
		NVVK_CHECK(vkQueueWaitIdle(queue));

		// --------
		// Clean up
		// --------
		vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
	}

	static void createBuffer(const nvvk::Context& _context, VkCommandBuffer& _cmdBuffer, 
								 const size_t _size,  
								 VkBuffer* buffer, VkBufferUsageFlags _bufferUsages, 
								 VkDeviceMemory* bufferMemory, VkMemoryPropertyFlags _memUsages)
	{
		// Create Buffer
		VkBufferCreateInfo bufferCreateInfo = nvvk::make<VkBufferCreateInfo>();
		bufferCreateInfo.size = _size;
		bufferCreateInfo.usage = _bufferUsages;
		NVVK_CHECK(vkCreateBuffer(_context.m_device, &bufferCreateInfo, nullptr, buffer));

		// Memory Requirement
		VkMemoryRequirements memReq;
		vkGetBufferMemoryRequirements(_context.m_device, *buffer, &memReq);
		

		// Find Memory Type
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_context.m_physicalDevice, &memProperties);
		uint32_t memTypeIndex;
		try
		{
			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				// memReq2.memoryRequirements.memoryTypeBits
				if ((memReq.memoryTypeBits & (1 << i))
					&&
					(memProperties.memoryTypes[i].propertyFlags & _memUsages) == _memUsages)
				{
					memTypeIndex = i;
				}
			}
		}
		catch (const std::exception&)
		{
			throw std::runtime_error("failed to find suitable memory type!");
		}

		auto memAllocateFlagsInfo = nvvk::make<VkMemoryAllocateFlagsInfo>();
		if (_bufferUsages & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			memAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
			memAllocateFlagsInfo.deviceMask = 0;
		}

		
		// Allocate Memory
		VkMemoryAllocateInfo memAllocateInfo = nvvk::make<VkMemoryAllocateInfo>();
		// memAllocateInfo.allocationSize = memReq2.memoryRequirements.size;
		memAllocateInfo.allocationSize	= memReq.size;
		memAllocateInfo.memoryTypeIndex = memTypeIndex;
		memAllocateInfo.pNext			= &memAllocateFlagsInfo;
		NVVK_CHECK(vkAllocateMemory(_context.m_device, &memAllocateInfo, nullptr, bufferMemory));

		// Bind buffer and memory
		vkBindBufferMemory(_context.m_device, *buffer, *bufferMemory, 0);
	}

	static void copyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize _size)
	{
		// this function require to begin a command record before this
		// and an end command record function afterward
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = _size;
		vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	}
}