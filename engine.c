/*
* Основной файл движка визуальных новелл
* Использует SDL2, OpenGL/Vulkan, SQLite, SDL_mixer, SDL_image, GLAD
*/
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <sqlite3.h>
#include <GLAD/glad.h>
#include <SDL_opengl.h>
#include <vulkan/vulkan.h>
#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <map>
#include <fstream>
#include <sstream>
#include <chrono>
#include <queue>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct VulcanImage {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet descriptorSet;
};

struct Vertex {
    float pos[2];
    float uv[2];
};

struct Transform {
    float x, y, w, h;
};

struct DisplayImage {
    std::string name;
    int x, y, w, h;
};

class VisualNovelEngine {
private:
    bool isRunning;
    bool useVulkan;
    int volume, pan;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GLContext glContext;
    VkInstance vkInstance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    uint32_t currentFrame = 0;
    sqlite3* db = nullptr;
    std::vector<std::thread> threads;
    std::mutex renderMutex;
    std::map<std::string, TTF_Font*> fonts;
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, Mix_Chunk*> sounds;
    std::map<std::string, SDL_Surface*> loadedSurfaces;
    std::map<std::string, VulcanImage> vulcanImages;
    std::queue<std::string> loadQueue;
    std::vector<std::string> script;
    std::map<std::string, size_t> scriptLabels;
    size_t currentCommand = 0;
    bool advanceStory = false;
    bool isWaitingForInput = false;
    std::vector<DisplayImage> imagesToDisplay;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDescriptorPool descriptorPool;
    const float windowWidth = 1920.0f;
    const float windowHeight = 1080.0f;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrameIndex = 0;
    std::map<std::string, int> variables;

    void logError(const std::string& msg, const std::string& errorDetails) {
        std::cerr << "[ERROR] " << msg << ": " << errorDetails << std::endl;
    }

    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            logError("Failed to open file", filename);
            throw std::runtime_error("Could not open file: " + filename);
        }
        size_t size = file.tellg();
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module;
        VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &module);
        if (result != VK_SUCCESS) {
            logError("Failed to create shader module", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create shader module");
        }
        return module;
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        logError("Failed to find suitable memory type", "No matching memory type found");
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            logError("Failed to create buffer", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
        if (result != VK_SUCCESS) {
            logError("Failed to allocate buffer memory", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        result = vkBindBufferMemory(device, buffer, bufferMemory, 0);
        if (result != VK_SUCCESS) {
            logError("Failed to bind buffer memory", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to bind buffer memory");
        }
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
        if (result != VK_SUCCESS) {
            logError("Failed to create image", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
        if (result != VK_SUCCESS) {
            logError("Failed to allocate image memory", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to allocate image memory");
        }

        result = vkBindImageMemory(device, image, imageMemory, 0);
        if (result != VK_SUCCESS) {
            logError("Failed to bind image memory", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to bind image memory");
        }
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            logError("Unsupported layout transition", "From " + std::to_string(oldLayout) + " to " + std::to_string(newLayout));
            throw std::runtime_error("Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(commandBuffer);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        if (result != VK_SUCCESS) {
            logError("Failed to allocate single-time command buffer", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to allocate command buffer");
        }

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS) {
            logError("Failed to begin single-time command buffer", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to begin command buffer");
        }
        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        VkResult result = vkEndCommandBuffer(commandBuffer);
        if (result != VK_SUCCESS) {
            logError("Failed to end single-time command buffer", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to end command buffer");
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            logError("Failed to submit single-time command buffer", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to submit command buffer");
        }
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        endSingleTimeCommands(commandBuffer);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
        if (result != VK_SUCCESS) {
            logError("Failed to create image view", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create image view");
        }
        return imageView;
    }

    VkSampler createSampler() {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
        if (result != VK_SUCCESS) {
            logError("Failed to create sampler", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create sampler");
        }
        return sampler;
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerLayoutBinding;

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            logError("Failed to create descriptor set layout", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create descriptor set layout");
        }
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 100; // Максимальное количество текстур

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 100;

        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        if (result != VK_SUCCESS) {
            logError("Failed to create descriptor pool", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create descriptor pool");
        }
    }

    VulcanImage createVulcanImageFromSurface(SDL_Surface* surface) {
        VulcanImage image;
        int width = surface->w;
        int height = surface->h;
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        createBuffer(width * height * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

        void* data;
        VkResult result = vkMapMemory(device, stagingMemory, 0, width * height * 4, 0, &data);
        if (result != VK_SUCCESS) {
            logError("Failed to map staging memory", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to map memory");
        }
        memcpy(data, surface->pixels, width * height * 4);
        vkUnmapMemory(device, stagingMemory);

        createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.image, image.memory);
        transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, image.image, width, height);
        transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        image.view = createImageView(image.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
        image.sampler = createSampler();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        result = vkAllocateDescriptorSets(device, &allocInfo, &image.descriptorSet);
        if (result != VK_SUCCESS) {
            logError("Failed to allocate descriptor set", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = image.view;
        imageInfo.sampler = image.sampler;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = image.descriptorSet;
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        return image;
    }

    void createVertexBuffer() {
        Vertex vertices[] = {
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f}, {1.0f, 0.0f}},
            {{1.0f, 1.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f}, {0.0f, 1.0f}}
        };
        uint16_t indices[] = {0, 1, 2, 2, 3, 0};

        VkDeviceSize vertexBufferSize = sizeof(vertices);
        VkDeviceSize indexBufferSize = sizeof(indices);

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

        void* data;
        vkMapMemory(device, stagingMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, vertices, vertexBufferSize);
        vkUnmapMemory(device, stagingMemory);

        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, vertexBufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);
        vkMapMemory(device, stagingMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, indices, indexBufferSize);
        vkUnmapMemory(device, stagingMemory);

        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        copyBuffer(stagingBuffer, indexBuffer, indexBufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion = {};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("vert.spv");
        auto fragShaderCode = readFile("frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attributeDescriptions[2] = {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, uv);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = 2;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = windowWidth;
        viewport.height = windowHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = {(uint32_t)windowWidth, (uint32_t)windowHeight};

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(Transform);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if (result != VK_SUCCESS) {
            logError("Failed to create pipeline layout", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
        if (result != VK_SUCCESS) {
            logError("Failed to create graphics pipeline", "Vulkan error code: " + std::to_string(result));
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    bool initVulkan() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Visual Novel Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VNE";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t extensionCount = 0;
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
        std::vector<const char*> extensions(extensionCount);
        SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
        createInfo.enabledExtensionCount = extensionCount;
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
        if (result != VK_SUCCESS) {
            logError("Failed to create Vulkan instance", "Vulkan error code: " + std::to_string(result));
            return false;
        }

        if (!SDL_Vulkan_CreateSurface(window, vkInstance, &surface)) {
            logError("Failed to create Vulkan surface", SDL_GetError());
            return false;
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            logError("Failed to find GPUs with Vulkan support", "No devices found");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            int graphicsFamily = -1, presentFamily = -1;
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) presentFamily = i;
                if (graphicsFamily != -1 && presentFamily != -1) break;
            }
            if (graphicsFamily != -1 && presentFamily != -1) {
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE) {
            logError("Failed to find suitable GPU", "No suitable device found");
            return false;
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = 0;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};
        const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        VkDeviceCreateInfo createInfoDevice = {};
        createInfoDevice.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfoDevice.pQueueCreateInfos = &queueCreateInfo;
        createInfoDevice.queueCreateInfoCount = 1;
        createInfoDevice.pEnabledFeatures = &deviceFeatures;
        createInfoDevice.enabledExtensionCount = 1;
        createInfoDevice.ppEnabledExtensionNames = deviceExtensions;

        result = vkCreateDevice(physicalDevice, &createInfoDevice, nullptr, &device);
        if (result != VK_SUCCESS) {
            logError("Failed to create logical device", "Vulkan error code: " + std::to_string(result));
            return false;
        }
        vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
        vkGetDeviceQueue(device, 0, 0, &presentQueue);

        VkSwapchainCreateInfoKHR createInfoSwapchain = {};
        createInfoSwapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfoSwapchain.surface = surface;
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
        createInfoSwapchain.minImageCount = capabilities.minImageCount + 1;
        createInfoSwapchain.imageFormat = formats[0].format;
        createInfoSwapchain.imageColorSpace = formats[0].colorSpace;
        createInfoSwapchain.imageExtent = capabilities.currentExtent;
        createInfoSwapchain.imageArrayLayers = 1;
        createInfoSwapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfoSwapchain.preTransform = capabilities.currentTransform;
        createInfoSwapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfoSwapchain.presentMode = presentModes[0];
        createInfoSwapchain.clipped = VK_TRUE;

        result = vkCreateSwapchainKHR(device, &createInfoSwapchain, nullptr, &swapchain);
        if (result != VK_SUCCESS) {
            logError("Failed to create swap chain", "Vulkan error code: " + std::to_string(result));
            return false;
        }

        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());
        swapchainImageViews.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++) {
            swapchainImageViews[i] = createImageView(swapchainImages[i], formats[0].format, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = formats[0].format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS) {
            logError("Failed to create render pass", "Vulkan error code: " + std::to_string(result));
            return false;
        }

        framebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &swapchainImageViews[i];
            framebufferInfo.width = capabilities.currentExtent.width;
            framebufferInfo.height = capabilities.currentExtent.height;
            framebufferInfo.layers = 1;

            result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]);
            if (result != VK_SUCCESS) {
                logError("Failed to create framebuffer", "Vulkan error code: " + std::to_string(result));
                return false;
            }
        }

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = 0;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS) {
            logError("Failed to create command pool", "Vulkan error code: " + std::to_string(result));
            return false;
        }

        commandBuffers.resize(imageCount);
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
        if (result != VK_SUCCESS) {
            logError("Failed to allocate command buffers", "Vulkan error code: " + std::to_string(result));
            return false;
        }

        createVertexBuffer();
        createDescriptorSetLayout();
        createDescriptorPool();
        createGraphicsPipeline();

        imageAvailableSemaphores.resize(2);
        renderFinishedSemaphores.resize(2);
        inFlightFences.resize(2);
        for (size_t i = 0; i < 2; i++) {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                logError("Failed to create synchronization objects", "Vulkan error");
                return false;
            }
        }

        return true;
    }

    bool initOpenGL() {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow("Visual Novel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if (!window) {
            logError("SDL_CreateWindow Error", SDL_GetError());
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            logError("SDL_CreateRenderer Error", SDL_GetError());
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            logError("SDL_GL_CreateContext Error", SDL_GetError());
            return false;
        }

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            logError("GLAD initialization failed", "Failed to initialize GLAD");
            return false;
        }

        glViewport(0, 0, 1920, 1080);
        return true;
    }

    bool initGraphics() {
        if (useVulkan) {
            window = SDL_CreateWindow("Visual Novel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
            if (!window) {
                logError("SDL_CreateWindow Error", SDL_GetError());
                return false;
            }
            return initVulkan();
        } else {
            return initOpenGL();
        }
    }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            logError("SDL_Init Error", SDL_GetError());
            return false;
        }
    
        if (TTF_Init() == -1) {
            logError("TTF_Init Error", TTF_GetError());
            return false;
        }
    
        if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
            logError("IMG_Init Error", IMG_GetError());
            return false;
        }
    
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            logError("Mix_OpenAudio Error", Mix_GetError());
            return false;
        }
        Mix_Volume(-1, MIX_MAX_VOLUME / 2); // Установить начальную громкость
        volume = MIX_MAX_VOLUME / 2;
        pan = 128; // Центр панорамы
    
        if (sqlite3_open("savegame.db", &db) != SQLITE_OK) {
            logError("SQLite open Error", sqlite3_errmsg(db));
            return false;
        }
        const char* createTableSQL = "CREATE TABLE IF NOT EXISTS savegame (id INTEGER PRIMARY KEY, state TEXT);";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            logError("SQLite create table Error", errMsg);
            sqlite3_free(errMsg);
            return false;
        }
    
        if (!initGraphics()) {
            logError("Graphics initialization failed", "Failed to initialize Vulkan or OpenGL");
            return false;
        }
    
        isRunning = true;
        threads.emplace_back(&VisualNovelEngine::loadResources, this);
    
        loadScript("script.txt"); // Загрузка начального сценария
        return true;
    }
    
    void loadResources() {
        while (isRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            std::lock_guard<std::mutex> lock(renderMutex);
            if (!loadQueue.empty()) {
                std::string resource = loadQueue.front();
                loadQueue.pop();
    
                if (textures.find(resource) == textures.end() && loadedSurfaces.find(resource) == loadedSurfaces.end()) {
                    SDL_Surface* surface = IMG_Load((resource + ".png").c_str());
                    if (surface) {
                        loadedSurfaces[resource] = surface;
                        if (useVulkan) {
                            vulcanImages[resource] = createVulcanImageFromSurface(surface);
                        } else {
                            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                            if (texture) {
                                textures[resource] = texture;
                            } else {
                                logError("Failed to create texture from surface", SDL_GetError());
                            }
                        }
                    } else {
                        logError("Image load failed", IMG_GetError());
                    }
                }
    
                if (sounds.find(resource) == sounds.end()) {
                    Mix_Chunk* sound = Mix_LoadWAV((resource + ".wav").c_str());
                    if (sound) {
                        sounds[resource] = sound;
                    } else {
                        logError("Sound load failed", Mix_GetError());
                    }
                }
            }
        }
    }
    
    void render() {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (useVulkan) {
            vkWaitForFences(device, 1, &inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);
    
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &imageIndex);
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                logError("Failed to acquire swapchain image", "Vulkan error code: " + std::to_string(result));
                return;
            }
    
            vkResetFences(device, 1, &inFlightFences[currentFrameIndex]);
            vkResetCommandBuffer(commandBuffers[imageIndex], 0);
    
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
            result = vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo);
            if (result != VK_SUCCESS) {
                logError("Failed to begin command buffer", "Vulkan error code: " + std::to_string(result));
                return;
            }
    
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = framebuffers[imageIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {(uint32_t)windowWidth, (uint32_t)windowHeight};
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
    
            vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
            VkBuffer vertexBuffers[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    
            for (const auto& img : imagesToDisplay) {
                if (vulcanImages.find(img.name) != vulcanImages.end()) {
                    Transform transform = {(float)img.x, (float)img.y, (float)img.w, (float)img.h};
                    vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Transform), &transform);
                    vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &vulcanImages[img.name].descriptorSet, 0, nullptr);
                    vkCmdDrawIndexed(commandBuffers[imageIndex], 6, 1, 0, 0, 0);
                }
            }
    
            vkCmdEndRenderPass(commandBuffers[imageIndex]);
            result = vkEndCommandBuffer(commandBuffers[imageIndex]);
            if (result != VK_SUCCESS) {
                logError("Failed to end command buffer", "Vulkan error code: " + std::to_string(result));
                return;
            }
    
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrameIndex]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrameIndex]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
    
            result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrameIndex]);
            if (result != VK_SUCCESS) {
                logError("Failed to submit draw command buffer", "Vulkan error code: " + std::to_string(result));
                return;
            }
    
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;
    
            result = vkQueuePresentKHR(presentQueue, &presentInfo);
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                logError("Failed to present swapchain image", "Vulkan error code: " + std::to_string(result));
                return;
            }
    
            currentFrameIndex = (currentFrameIndex + 1) % 2;
        } else {
            SDL_RenderClear(renderer);
            for (const auto& img : imagesToDisplay) {
                if (textures.find(img.name) != textures.end()) {
                    SDL_Rect rect = {img.x, img.y, img.w, img.h};
                    SDL_RenderCopy(renderer, textures[img.name], nullptr, &rect);
                }
            }
            SDL_RenderPresent(renderer);
        }
    }
    
    void showImage(const std::string& imageName, int x, int y, int w, int h) {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (useVulkan && vulcanImages.find(imageName) == vulcanImages.end() ||
            !useVulkan && textures.find(imageName) == textures.end()) {
            loadQueue.push(imageName);
        }
        imagesToDisplay.push_back({imageName, x, y, w, h});
    }
    
    void renderText(const std::string& fontName, const std::string& text, int x, int y, SDL_Color color) {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (fonts.find(fontName) == fonts.end()) {
            TTF_Font* font = TTF_OpenFont("font.ttf", 24);
            if (!font) {
                logError("Failed to load font", TTF_GetError());
                return;
            }
            fonts[fontName] = font;
        }
        TTF_Font* font = fonts[fontName];
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!textSurface) {
            logError("Failed to render text", TTF_GetError());
            return;
        }
    
        std::string textKey = "text_" + std::to_string(x) + "_" + std::to_string(y);
        if (useVulkan) {
            vulcanImages[textKey] = createVulcanImageFromSurface(textSurface);
            imagesToDisplay.push_back({textKey, x, y, textSurface->w, textSurface->h});
        } else {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (!textTexture) {
                logError("Failed to create text texture", SDL_GetError());
                SDL_FreeSurface(textSurface);
                return;
            }
            textures[textKey] = textTexture;
            imagesToDisplay.push_back({textKey, x, y, textSurface->w, textSurface->h});
        }
        SDL_FreeSurface(textSurface);
    }
    
    void playSound(const std::string& soundName) {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (sounds.find(soundName) != sounds.end()) {
            int channel = Mix_PlayChannel(-1, sounds[soundName], 0);
            if (channel != -1) {
                Mix_Volume(channel, volume);
                Mix_SetPanning(channel, pan, 255 - pan);
            } else {
                logError("Failed to play sound", Mix_GetError());
            }
        } else {
            loadQueue.push(soundName);
        }
    }
    
    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
                if (!isWaitingForInput) {
                    advanceStory = true;
                } else {
                    isWaitingForInput = false;
                    advanceStory = true;
                }
            }
        }
    }
    
    void executeCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmdType;
        iss >> cmdType;
    
        if (cmdType == "show") {
            std::string resourceType, resourceName;
            iss >> resourceType >> resourceName;
            if (resourceType == "image") {
                showImage(resourceName, 0, 0, 1920, 1080);
            }
        } else if (cmdType == "say") {
            std::string text;
            std::getline(iss, text);
            text = text.substr(text.find_first_not_of(" "));
            renderText("default", text, 100, 900, {255, 255, 255, 255});
            isWaitingForInput = true;
        } else if (cmdType == "play") {
            std::string resourceType, soundName;
            iss >> resourceType >> soundName;
            if (resourceType == "sound") {
                playSound(soundName);
            }
        } else if (cmdType == "choice") {
            std::vector<std::pair<std::string, std::string>> choices;
            std::string choiceStr;
            std::getline(iss, choiceStr);
            std::istringstream choiceIss(choiceStr);
            std::string option, label;
            while (std::getline(choiceIss, option, ',') && std::getline(choiceIss, label, ',')) {
                option = option.substr(option.find_first_not_of(" "));
                label = label.substr(label.find_first_not_of(" "));
                choices.push_back({option, label});
                renderText("default", option, 100, 900 + (choices.size() - 1) * 50, {255, 255, 255, 255});
            }
            // Здесь нужно добавить логику выбора пользователем (например, через handleEvents)
            isWaitingForInput = true;
        } else if (cmdType == "loop") {
            int count;
            iss >> count;
            std::string loopCommand;
            std::getline(iss, loopCommand);
            for (int i = 0; i < count; ++i) {
                executeCommand(loopCommand);
            }
        } else if (cmdType == "if") {
            std::string condition, thenCommand;
            iss >> condition;
            std::getline(iss, thenCommand);
            // Простая проверка условия (например, "var > 5")
            std::string varName = condition.substr(0, condition.find(' '));
            char op = condition[condition.find(' ') + 1];
            int value = std::stoi(condition.substr(condition.find(' ') + 3));
            if (variables.find(varName) != variables.end()) {
                int varValue = variables[varName];
                if ((op == '>' && varValue > value) || (op == '<' && varValue < value) || (op == '=' && varValue == value)) {
                    executeCommand(thenCommand);
                }
            }
        } else if (cmdType == "set") {
            std::string varName;
            int value;
            iss >> varName >> value;
            variables[varName] = value;
        } else if (cmdType == "label") {
            std::string labelName;
            iss >> labelName;
            scriptLabels[labelName] = currentCommand;
        } else if (cmdType == "goto") {
            std::string labelName;
            iss >> labelName;
            if (scriptLabels.find(labelName) != scriptLabels.end()) {
                currentCommand = scriptLabels[labelName];
            }
        }
    }
    
    void loadScript(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            logError("Failed to open script file", filename);
            return;
        }
        script.clear();
        scriptLabels.clear();
        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            script.push_back(line);
            std::istringstream iss(line);
            std::string cmdType;
            iss >> cmdType;
            if (cmdType == "label") {
                std::string labelName;
                iss >> labelName;
                scriptLabels[labelName] = lineNum;
            }
            lineNum++;
        }
        file.close();
    }
    
    void saveGame() {
        std::lock_guard<std::mutex> lock(renderMutex);
        json state;
        state["currentCommand"] = currentCommand;
        state["images"] = json::array();
        for (const auto& img : imagesToDisplay) {
            state["images"].push_back({{"name", img.name}, {"x", img.x}, {"y", img.y}, {"w", img.w}, {"h", img.h}});
        }
        state["variables"] = variables;
    
        std::string stateStr = state.dump();
        sqlite3_stmt* stmt;
        std::string sql = "INSERT OR REPLACE INTO savegame (id, state) VALUES (1, ?);";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, stateStr.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                logError("SQLite save Error", sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
        } else {
            logError("SQLite prepare Error", sqlite3_errmsg(db));
        }
    }
    
    void loadGame() {
        std::lock_guard<std::mutex> lock(renderMutex);
        sqlite3_stmt* stmt;
        std::string sql = "SELECT state FROM savegame WHERE id = 1;";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* stateStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                json state = json::parse(stateStr);
                currentCommand = state["currentCommand"];
                imagesToDisplay.clear();
                for (const auto& img : state["images"]) {
                    imagesToDisplay.push_back({img["name"], img["x"], img["y"], img["w"], img["h"]});
                }
                variables = state["variables"].get<std::map<std::string, int>>();
            }
            sqlite3_finalize(stmt);
        } else {
            logError("SQLite load Error", sqlite3_errmsg(db));
        }
    }
    
    void run() {
        while (isRunning) {
            handleEvents();
            if (advanceStory && currentCommand < script.size()) {
                executeCommand(script[currentCommand]);
                if (!isWaitingForInput) {
                    currentCommand++;
                    advanceStory = false;
                }
            }
            render();
        }
    }
    
    void cleanup() {
        isRunning = false;
        for (auto& thread : threads) {
            if (thread.joinable()) thread.join();
        }
    
        for (auto& texture : textures) {
            SDL_DestroyTexture(texture.second);
        }
        for (auto& sound : sounds) {
            Mix_FreeChunk(sound.second);
        }
        for (auto& font : fonts) {
            TTF_CloseFont(font.second);
        }
        for (auto& surface : loadedSurfaces) {
            SDL_FreeSurface(surface.second);
        }
        if (useVulkan) {
            for (size_t i = 0; i < 2; i++) {
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }
            vkDestroyCommandPool(device, commandPool, nullptr);
            for (auto& fb : framebuffers) {
                vkDestroyFramebuffer(device, fb, nullptr);
            }
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            vkDestroyRenderPass(device, renderPass, nullptr);
            for (auto& imageView : swapchainImageViews) {
                vkDestroyImageView(device, imageView, nullptr);
            }
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);
            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);
            for (auto& img : vulcanImages) {
                vkDestroySampler(device, img.second.sampler, nullptr);
                vkDestroyImageView(device, img.second.view, nullptr);
                vkDestroyImage(device, img.second.image, nullptr);
                vkFreeMemory(device, img.second.memory, nullptr);
            }
            vkDestroyDevice(device, nullptr);
            vkDestroySurfaceKHR(vkInstance, surface, nullptr);
            vkDestroyInstance(vkInstance, nullptr);
        } else {
            SDL_GL_DeleteContext(glContext);
            SDL_DestroyRenderer(renderer);
        }
        if (db) sqlite3_close(db);
        if (window) SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
    }
    
    public:
        VisualNovelEngine(bool useVulkan_ = true) : isRunning(false), useVulkan(useVulkan_) {}
        ~VisualNovelEngine() { cleanup(); }
    
        bool start() {
            if (!init()) return false;
            run();
            return true;
        }
    };
    
    int main() {
        VisualNovelEngine engine(true); // Использовать Vulkan
        if (!engine.start()) {
            std::cerr << "Engine failed to start" << std::endl;
            return 1;
        }
        return 0;
    }

       