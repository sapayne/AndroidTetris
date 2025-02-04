// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanMain.hpp"
#include <vulkan_wrapper.h>


#include <android/log.h>

#include <cassert>
#include <cstring>
#include <vector>

//

//added includes
//#include "Color.h" //no longer needed
#include "glm/glm.hpp"
#include <array>

// Android log function wrappers
static const char* kTAG = "Tetris";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Tetris ",               \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

// Global Variables ...
struct VulkanDeviceInfo {
	bool initialized_;

	VkInstance instance_;
	VkPhysicalDevice gpuDevice_;
	VkDevice device_;
	uint32_t queueFamilyIndex_;

	VkSurfaceKHR surface_;
	VkQueue queue_;
};
VulkanDeviceInfo device;

struct VulkanSwapchainInfo {
	VkSwapchainKHR swapchain_;
	uint32_t swapchainLength_;

	VkExtent2D displaySize_;
	VkFormat displayFormat_;

	// array of frame buffers and views
	std::vector<VkImage> displayImages_;
	std::vector<VkImageView> displayViews_;
	std::vector<VkFramebuffer> framebuffers_;
};
VulkanSwapchainInfo swapchain;

struct VulkanBufferInfo {
	VkBuffer vertexBuf_;
};
VulkanBufferInfo buffers;

struct VulkanGfxPipelineInfo {
	VkPipelineLayout layout_;
	VkPipelineCache cache_;
	VkPipeline pipeline_;
};
VulkanGfxPipelineInfo gfxPipeline;

struct VulkanRenderInfo {
	VkRenderPass renderPass_;
	VkCommandPool cmdPool_;
	VkCommandBuffer* cmdBuffer_;
	uint32_t cmdBufferLen_;
	VkSemaphore semaphore_;
	VkFence fence_;
};
VulkanRenderInfo render;

// Android Native App pointer...
android_app* androidAppCtx = nullptr;

//added variables -----------------------------------------------------------------------------------
//Color Variables
glm::vec3 Magenta = {1,0,1}, Yellow = {1,1,0}, Cyan = {0,1,1}, Blue = {0,0,1}, Orange = {1,.5,0}, Red = {1,0,0}, Green = {0,1,0};

// current piece goes from 1 to 7 and is used to determine what color the squares of the piece are
int currentPiece;
// current piece side size is the square root of the length of the array containing the structure of the current piece
int currentPieceSideSize;
// the top left coordinates of the current piece
int currentPieceX, currentPieceY;
// the currentPiece is whichever rotation it is
int* currentPieceRotation;
//keeps track of the rows that are full and need to be removed
int rowsToDelete[4] = {-1};
//stores the x and y offsets from the top left coordinate
int currentBoundingBox[4][2];
// the brackets initializes the elements of the array to 0
int rowSize[20] = {};
int gameboard[20][10] = {};
bool gameboardStateChanged = false;

//game piece variables
static int tPiece[9] = {0,1,0,
				 		1,1,1,
				 		0,0,0},
			boxPiece[4] = {2,2,
						   2,2},
			iPiece[16] = {0,0,0,0,
						  3,3,3,3,
						  0,0,0,0,
						  0,0,0,0},
			rightLPiece[9] = {4,0,0,
							  4,4,4,
							  0,0,0},
			lPiece[9] = {0,0,5,
						 5,5,5,
						 0,0,0},
			lIsomerPiece[9] = {6,6,0,
							   0,6,6,
							   0,0,0},
			rIsomerPiece[9] = {0,7,7,
							   7,7,0,
							   0,0,0};
// end of added variables ---------------------------------------------------------------------------

//added function
void genBoundingBox(){
	int count = 0;
	for(int i = 0; i < currentPieceSideSize * currentPieceSideSize; i++){
		if(currentPieceRotation[i] != 0){
			// outputs the x offset
			currentBoundingBox[count][0] = i % currentPieceSideSize;
			// outputs the y offset
			currentBoundingBox[count][1] = i / currentPieceSideSize;
			count++;
			if(count == 4) {
				break;
			}
		}
	}
}

//added function
void genNextPiece(){
	currentPiece = rand() % 7 + 1;
	currentPieceY = 19;
	currentPieceX = 4;
	switch(currentPiece){
		case 1:
			currentPieceRotation = tPiece;
			currentPieceSideSize = 3;
			break;
		case 2:
			currentPieceRotation = boxPiece;
			currentPieceSideSize = 2;
			break;
		case 3:
			currentPieceRotation = iPiece;
			currentPieceSideSize = 4;
			currentPieceY++;
			currentPieceX--;
			break;
		case 4:
			currentPieceRotation = rightLPiece;
			currentPieceSideSize = 3;
			break;
		case 5:
			currentPieceRotation = lPiece;
			currentPieceSideSize = 3;
			break;
		case 6:
			currentPieceRotation = lIsomerPiece;
			currentPieceSideSize = 3;
			break;
		case 7:
			currentPieceRotation = rIsomerPiece;
			currentPieceSideSize = 3;
			break;
		default:
			break;
	}
	genBoundingBox();
}

//added function
//clockwise rotation of the current game piece
bool clockwiseRotation() {
	if (currentPiece != 2) {
		// counter clockwise rotation Inplace function found on geeksforgeeks.org/inplace-rotate-square-matrix-by-90-degrees/
		// then rewritten to work for arrays and clockwise rotation by reversing the order the indices are exchanged
		for (int i = 0; i < currentPieceSideSize / 2; i++) {
			for (int j = i; j < currentPieceSideSize - i - 1; j++) {
				int temp = currentPieceRotation[(currentPieceSideSize - 1 - j) * currentPieceSideSize + i];
				currentPieceRotation[(currentPieceSideSize - 1 - j) * currentPieceSideSize + i] = currentPieceRotation[(currentPieceSideSize - 1 - i) * currentPieceSideSize + currentPieceSideSize - 1 - j];
				currentPieceRotation[(currentPieceSideSize - 1 - i) * currentPieceSideSize + currentPieceSideSize - 1 - j] = currentPieceRotation[j * currentPieceSideSize + currentPieceSideSize - 1 - i];
				currentPieceRotation[j * currentPieceSideSize + currentPieceSideSize - 1 - i] = currentPieceRotation[i * currentPieceSideSize + j];
				currentPieceRotation[i * currentPieceSideSize + j] = temp;
			}
		}
		genBoundingBox();
	}
	return true;
}

//added function
//counter clockwise rotation of the current game piece
bool cClockwiseRotation(){
	if(currentPiece != 2){
		// counter clockwise rotation Inplace function found on geeksforgeeks.org/inplace-rotate-square-matrix-by-90-degrees/
		// then rewritten to work for arrays
		for(int i = 0; i < currentPieceSideSize / 2; i++){
			for(int j = i; j < currentPieceSideSize - i - 1; j++){
				int temp = currentPieceRotation[i * currentPieceSideSize + j];
				currentPieceRotation[i * currentPieceSideSize + j] = currentPieceRotation[j * currentPieceSideSize + currentPieceSideSize - 1 - i];
				currentPieceRotation[j * currentPieceSideSize + currentPieceSideSize - 1 - i] = currentPieceRotation[(currentPieceSideSize - 1 - i) * currentPieceSideSize + currentPieceSideSize - 1 - j];
				currentPieceRotation[(currentPieceSideSize - 1 - i) * currentPieceSideSize + currentPieceSideSize - 1 - j] = currentPieceRotation[(currentPieceSideSize - 1 - j) * currentPieceSideSize + i];
				currentPieceRotation[(currentPieceSideSize - 1 - j) * currentPieceSideSize + i] = temp;
			}
		}
		genBoundingBox();
	}
	return true;
}

// added function
// this is how far on the board the piece should move, check for collisions with pieces currently placed in the board, also use for the current piece progressively moving down
void translate(int x, int y){
	//get the current piece's position on the board and then move the piece based on x and y but test for collision each time the piece moves.
	int tempX, tempY;
	// assume that every move is valid from the start
	bool movementTest = true, rowFull = false;
	// write 2 for loops one for the x translation and one for the y translation
	if(x < 0){
		for(int i = 0; i > x; i--){
			//if the space to the left of each square of the current piece is a zero on the board move left
			for(int j = 0; j < 4; j++){
				tempX = currentPieceX + currentBoundingBox[j][0] - 1;
				if(tempX < 0 || gameboard[currentPieceY - currentBoundingBox[j][1]][tempX] != 0){
					movementTest = false;
					break;
				}
			}
			if(movementTest){
				currentPieceX -= 1;
			} else {
				break;
			}
		}
	} else {
		for(int i = 0; i < x; i++){
			//if the space to the right of each square of the current piece is a zero on the board move right
			for(int j = 0; j < 4; j++){
				tempX = currentPieceX + currentBoundingBox[j][0] + 1;
				if(tempX > 9 || gameboard[currentPieceY - currentBoundingBox[j][1]][tempX] != 0){
					movementTest = false;
					break;
				}
			}
			if(movementTest){
				currentPieceX += 1;
			} else {
				break;
			}
		}
	}
	for(int i = 0; i < y; i++){
		// if the space to the down of each square of the current piece is a zero on the board move
		// down, when the piece can no longer move down state the piece to the board
		for(int j = 0; j < 4; j++){
			tempY = currentPieceY - currentBoundingBox[j][1] - 1;
			if(tempY < 0 || gameboard[tempY][currentPieceX + currentBoundingBox[j][0]] != 0){
				movementTest = false;
			}
		}
		if(movementTest){
			currentPieceY -= 1;
		} else {
			// write contents to the game board
			for(int j = 0; j < 4; j++){
				tempY = currentPieceY - currentBoundingBox[j][1];
				tempX = currentPieceX + currentBoundingBox[j][0];
				gameboard[tempY][tempX] = currentPiece;
				if(++rowSize[tempY] == 10){
					rowFull = true;
					// because the pointer to the array is found at the top left of the piece, when
					// the rows are added to the rowsToDelete array, are the higher up rows first
					rowsToDelete[j] = tempY;
				}
			}
			if(rowFull){
				int row;
				for(int j = 0; j < 4; j++){
					/* 	because the higher located rows are stored first this actually helps us when
						shifting the rows down to erase the current row that scored a point as if it
						were the other way. if two rows that have 10 elements are on top of each other
						the row on the bottom will copy the top row contents onto itself which means
						the index that was saved for the row above will now have a non-filled row so
						when the deletion code goes to delete the row it deletes the wrong row
					*/
					row = rowsToDelete[j];
					if(row > -1){
						// used to optimize the number of elements that need to be rewritten
						// k has to go to one less than the height of the board because we don't
						// check if the row above is valid
						for(int k = row; k < 19; k++){
							for(int l = 0; l < 10; l++){
								gameboard[row][l] = gameboard[row + 1][l];
								rowSize[row] = rowSize[row + 1];
							}
							// if the next row doesn't have any pieces' squares in it then the row
							// above also won't have anything in it. Used for optimization
							if(rowSize[k] == 0){
								break;
							}
						}
					}
				}
			}
			// make new piece after the last piece has been written into the game board
			genNextPiece();
			gameboardStateChanged = true;
			break;
		}
	}

}


// vertex code taken from vulkan-tutorial.com/code/17_vertex_input.cpp
// used to packet the position and color to be sent to the gpu
struct Vertex {
	glm::vec3 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDesc(){
		VkVertexInputBindingDescription bindingDescription = {
				.binding = 0,
				.stride = sizeof(Vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributes(){
		std::array<VkVertexInputAttributeDescription, 2> attributes = {};
		attributes[0].binding = 0;
		attributes[0].location = 0;
		attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributes[0].offset = offsetof(Vertex,position);

		attributes[1].binding = 0;
		attributes[1].location = 1;
		attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributes[1].offset = offsetof(Vertex,color);

		return attributes;
	}
};

std::vector<Vertex> vertices;

// added function
void squareCoords(int x, int y, int pieceType){
	glm::vec3 Color;
	switch (pieceType){
		case 1:
			Color = Magenta;
			break;
		case 2:
			Color = Yellow;
			break;
		case 3:
			Color = Cyan;
			break;
		case 4:
			Color = Blue;
			break;
		case 5:
			Color = Orange;
			break;
		case 6:
			Color = Red;
			break;
		case 7:
			Color = Green;
			break;
		default:
			break;
	}
	double xCoord = -1 + .2 * x, yCoord = .9 - .1 * y;
	Vertex temp[6] = {
			{{0.0f + xCoord, 0.0f + yCoord, 0.0f}, Color},
			{{0.2f + xCoord, 0.0f + yCoord, 0.0f}, Color},
			{{0.0f + xCoord, 0.1f + yCoord, 0.0f}, Color},
			{{0.2f + xCoord, 0.0f + yCoord, 0.0f}, Color},
			{{0.2f + xCoord, 0.1f + yCoord, 0.0f}, Color},
			{{0.0f + xCoord, 0.1f + yCoord, 0.0f}, Color}
	};
	vertices.insert(vertices.end(), temp, temp +6);
}

// added function
void removeCurrentPiece(){
	if(vertices.size() > 0) {
		vertices.erase(vertices.end()-24, vertices.end());
	}
}

// added function
void addCurrentPiece(){
	for(int i = 0; i < 4; i++){
		//currentPieceY - currentBoundingBox[i][1] // Y Coord for square of piece
		//currentPieceX + currentBoundingBox[i][0] // X Coord for square of piece
		squareCoords(currentPieceX + currentBoundingBox[i][0], currentPieceY - currentBoundingBox[i][1], currentPiece);
	}
}

// added function
// checking the state of the board and passing the non-empty indices positions to the squareCoords()
// to then compute the position on the screen at which to draw the square with the appropriate color
void gameboardStateToDraw() {
	int count, piece;
	for(int i = 0; i < 20; i++){
		if(rowSize[i] > 0){
			count = 0;
			for(int j = 0; j < 10; j++){
				if(rowSize[i] == 0){
					break;
				}
				piece = gameboard[i][j];
				if(piece > 0){
					count++;
					squareCoords(j, i, piece);
					if(count == rowSize[i]){
						break;
					}
				}
			}
		}
	}
}

/*
 * setImageLayout():
 *    Helper function to transition color buffer layout
 */
void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
					VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
					VkPipelineStageFlags srcStages,
					VkPipelineStageFlags destStages);

// Create vulkan device
void CreateVulkanDevice(ANativeWindow* platformWindow, VkApplicationInfo* appInfo) {
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

	instance_extensions.push_back("VK_KHR_surface");
	instance_extensions.push_back("VK_KHR_android_surface");

	device_extensions.push_back("VK_KHR_swapchain");

	// **********************************************************
	// Create the Vulkan instance
	VkInstanceCreateInfo instanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.pApplicationInfo = appInfo,
			.enabledExtensionCount =
			static_cast<uint32_t>(instance_extensions.size()),
			.ppEnabledExtensionNames = instance_extensions.data(),
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
	};
	CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &device.instance_));
	VkAndroidSurfaceCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.window = platformWindow};

	CALL_VK(vkCreateAndroidSurfaceKHR(device.instance_, &createInfo, nullptr,
									  &device.surface_));
	// Find one GPU to use:
	// On Android, every GPU device is equal -- supporting
	// graphics/compute/present
	// for this sample, we use the very first GPU device found on the system
	uint32_t gpuCount = 0;
	CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, nullptr));
	if(gpuCount == 0){
		throw std::runtime_error("failed to find GPUs with Vulkan Support.");
	}
	VkPhysicalDevice tmpGpus[gpuCount];
	CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, tmpGpus));
	device.gpuDevice_ = tmpGpus[0];  // Pick up the first GPU Device

	// Find a GFX queue family
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device.gpuDevice_, &queueFamilyCount,
											 nullptr);
	assert(queueFamilyCount);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device.gpuDevice_, &queueFamilyCount,
											 queueFamilyProperties.data());

	uint32_t queueFamilyIndex;
	for (queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount;
		 queueFamilyIndex++) {
		if (queueFamilyProperties[queueFamilyIndex].queueFlags &
			VK_QUEUE_GRAPHICS_BIT) {
			break;
		}
	}
	assert(queueFamilyIndex < queueFamilyCount);
	device.queueFamilyIndex_ = queueFamilyIndex;

	// Create a logical device (vulkan device)
	float priorities[] = {
			1.0f,
	};
	VkDeviceQueueCreateInfo queueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCount = 1,
			.queueFamilyIndex = queueFamilyIndex,
			.pQueuePriorities = priorities,
	};

	VkDeviceCreateInfo deviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueCreateInfo,
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = nullptr,
			.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
			.ppEnabledExtensionNames = device_extensions.data(),
			.pEnabledFeatures = nullptr,
	};

	CALL_VK(vkCreateDevice(device.gpuDevice_, &deviceCreateInfo, nullptr,
						   &device.device_));
	vkGetDeviceQueue(device.device_, device.queueFamilyIndex_, 0, &device.queue_);
}

void CreateSwapChain(void) {
	LOGI("->createSwapChain");
	memset(&swapchain, 0, sizeof(swapchain));

	// **********************************************************
	// Get the surface capabilities because:
	//   - It contains the minimal and max length of the chain, we will need it
	//   - It's necessary to query the supported surface format (R8G8B8A8 for
	//   instance ...)
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.gpuDevice_, device.surface_,
											  &surfaceCapabilities);
	// Query the list of supported surface format and choose one we like
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpuDevice_, device.surface_,
										 &formatCount, nullptr);
	VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[formatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpuDevice_, device.surface_,
										 &formatCount, formats);
	LOGI("Got %d formats", formatCount);

	uint32_t chosenFormat;
	for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
		if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break;
	}
	assert(chosenFormat < formatCount);

	swapchain.displaySize_ = surfaceCapabilities.currentExtent;
	swapchain.displayFormat_ = formats[chosenFormat].format;

	VkSurfaceCapabilitiesKHR surfaceCap;
	CALL_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.gpuDevice_,
													  device.surface_, &surfaceCap));
	assert(surfaceCap.supportedCompositeAlpha | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

	// **********************************************************
	// Create a swap chain (here we choose the minimum available number of surface
	// in the chain)
	VkSwapchainCreateInfoKHR swapchainCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.surface = device.surface_,
			.minImageCount = surfaceCapabilities.minImageCount,
			.imageFormat = formats[chosenFormat].format,
			.imageColorSpace = formats[chosenFormat].colorSpace,
			.imageExtent = surfaceCapabilities.currentExtent,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.imageArrayLayers = 1,
			.imageSharingMode = VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &device.queueFamilyIndex_,
			.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.oldSwapchain = VK_NULL_HANDLE,
			.clipped = VK_TRUE,
	};
	CALL_VK(vkCreateSwapchainKHR(device.device_, &swapchainCreateInfo, nullptr,
								 &swapchain.swapchain_));

	// Get the length of the created swap chain
	CALL_VK(vkGetSwapchainImagesKHR(device.device_, swapchain.swapchain_,
									&swapchain.swapchainLength_, nullptr));
	delete[] formats;
	LOGI("<-createSwapChain");
}

void DeleteSwapChain(void) {
	for (int i = 0; i < swapchain.swapchainLength_; i++) {
		vkDestroyFramebuffer(device.device_, swapchain.framebuffers_[i], nullptr);
		vkDestroyImageView(device.device_, swapchain.displayViews_[i], nullptr);
		vkDestroyImage(device.device_, swapchain.displayImages_[i], nullptr);
	}
	vkDestroySwapchainKHR(device.device_, swapchain.swapchain_, nullptr);
}

void CreateFrameBuffers(VkRenderPass& renderPass, VkImageView depthView = VK_NULL_HANDLE) {
	// query display attachment to swapchain
	uint32_t SwapchainImagesCount = 0;
	CALL_VK(vkGetSwapchainImagesKHR(device.device_, swapchain.swapchain_, &SwapchainImagesCount, nullptr));
	swapchain.displayImages_.resize(SwapchainImagesCount);
	CALL_VK(vkGetSwapchainImagesKHR(device.device_, swapchain.swapchain_, &SwapchainImagesCount, swapchain.displayImages_.data()));

	// create image view for each swapchain image
	swapchain.displayViews_.resize(SwapchainImagesCount);
	for (uint32_t i = 0; i < SwapchainImagesCount; i++) {
		VkImageViewCreateInfo viewCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.image = swapchain.displayImages_[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapchain.displayFormat_,
				.components =
						{
								.r = VK_COMPONENT_SWIZZLE_R,
								.g = VK_COMPONENT_SWIZZLE_G,
								.b = VK_COMPONENT_SWIZZLE_B,
								.a = VK_COMPONENT_SWIZZLE_A,
						},
				.subresourceRange =
						{
								.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
								.baseMipLevel = 0,
								.levelCount = 1,
								.baseArrayLayer = 0,
								.layerCount = 1,
						},
				.flags = 0,
		};
		CALL_VK(vkCreateImageView(device.device_, &viewCreateInfo, nullptr, &swapchain.displayViews_[i]));
	}
	// create a framebuffer from each swapchain image
	swapchain.framebuffers_.resize(swapchain.swapchainLength_);
	for (uint32_t i = 0; i < swapchain.swapchainLength_; i++) {
		VkImageView attachments[2] = {
				swapchain.displayViews_[i], depthView,
		};
		VkFramebufferCreateInfo fbCreateInfo{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext = nullptr,
				.renderPass = renderPass,
				.layers = 1,
				.attachmentCount = 1,  // 2 if using depth
				.pAttachments = attachments,
				.width = static_cast<uint32_t>(swapchain.displaySize_.width),
				.height = static_cast<uint32_t>(swapchain.displaySize_.height),
		};
		fbCreateInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2);

		CALL_VK(vkCreateFramebuffer(device.device_, &fbCreateInfo, nullptr, &swapchain.framebuffers_[i]));
	}
}

// A helper function
bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device.gpuDevice_, &memoryProperties);
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < 32; i++) {
		if ((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) ==
				requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

// added function
void updateVertexData(){
	// remove the vertices of the current piece from the draw vector
	removeCurrentPiece();

	// for testing the new renderer setup, works
	// TODO write method to update the vector to contain the game board state and the current piece
	// test if the gameboard state has changed since last call
	if(gameboardStateChanged){
		vertices.clear();
		gameboardStateToDraw();
		gameboardStateChanged = false;
	}

	// add the vertices of the current piece to the draw vector
	addCurrentPiece();
}

// Create our vertex buffer
bool CreateBuffers() {
	// -----------------------------------------------
	// Create the triangle vertex buffer
	// replace the 0s with x and y values to update position
	/*
	float vertexData[] = {
			0.0f + x, 0.0f + y, 0.0f,
			0.18f + x, 0.0f + y, 0.0f,
			0.0f + x, 0.09f + y, 0.0f,
			0.18f + x, 0.0f + y, 0.0f,
			0.0f + x, 0.09f + y, 0.0f,
			0.18f + x, 0.09f + y, 0.0f,
	};*/

	// the vertex data is taken care of by the the new function below
	updateVertexData();

	// Create a vertex buffer
	VkBufferCreateInfo createBufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.size = sizeof(vertices[0]) * vertices.size(),//vertexData),
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.flags = 0,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.pQueueFamilyIndices = &device.queueFamilyIndex_,
			.queueFamilyIndexCount = 1,
	};

	CALL_VK(vkCreateBuffer(device.device_, &createBufferInfo, nullptr, &buffers.vertexBuf_));

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device.device_, buffers.vertexBuf_, &memReq);

	VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memReq.size,
			.memoryTypeIndex = 0,  // Memory type assigned in the next step
	};

	// Assign the proper memory type for that buffer
	MapMemoryTypeToIndex(memReq.memoryTypeBits,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						 &allocInfo.memoryTypeIndex);

	// Allocate memory for the buffer
	VkDeviceMemory deviceMemory;
	CALL_VK(vkAllocateMemory(device.device_, &allocInfo, nullptr, &deviceMemory));

	void* data;
	CALL_VK(vkMapMemory(device.device_, deviceMemory, 0, allocInfo.allocationSize, 0, &data));
	//memcpy(data, vertexData, sizeof(vertexData));
	// changed the third parameter from sizeof(vertices), and the updatable colored triangles now work
	memcpy(data, vertices.data(), (size_t) allocInfo.allocationSize);
	vkUnmapMemory(device.device_, deviceMemory);

	CALL_VK(vkBindBufferMemory(device.device_, buffers.vertexBuf_, deviceMemory, 0));
	return true;
}

void DeleteBuffers(void) {
	vkDestroyBuffer(device.device_, buffers.vertexBuf_, nullptr);
	//vkFreeMemory(device.device_, deviceMemory, nullptr);
}

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };
VkResult loadShaderFromFile(const char* filePath, VkShaderModule* shaderOut, ShaderType type) {
	// Read the file
	assert(androidAppCtx);
	AAsset* file = AAssetManager_open(androidAppCtx->activity->assetManager,
									  filePath, AASSET_MODE_BUFFER);
	size_t fileLength = AAsset_getLength(file);

	char* fileContent = new char[fileLength];

	AAsset_read(file, fileContent, fileLength);
	AAsset_close(file);

	VkShaderModuleCreateInfo shaderModuleCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.codeSize = fileLength,
			.pCode = (const uint32_t*)fileContent,
			.flags = 0,
	};
	VkResult result = vkCreateShaderModule(
			device.device_, &shaderModuleCreateInfo, nullptr, shaderOut);
	assert(result == VK_SUCCESS);

	delete[] fileContent;

	return result;
}

// Create Graphics Pipeline
VkResult CreateGraphicsPipeline(void) {
	memset(&gfxPipeline, 0, sizeof(gfxPipeline));
	// Create pipeline layout (empty)
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
	};
	CALL_VK(vkCreatePipelineLayout(device.device_, &pipelineLayoutCreateInfo,
								   nullptr, &gfxPipeline.layout_));

	VkShaderModule vertexShader, fragmentShader;
	loadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
	loadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);

	// Specify vertex and fragment shader stages
	VkPipelineShaderStageCreateInfo shaderStages[2]{
			{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = vertexShader,
					.pSpecializationInfo = nullptr,
					.flags = 0,
					.pName = "main",
			},
			{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = fragmentShader,
					.pSpecializationInfo = nullptr,
					.flags = 0,
					.pName = "main",
			}};

	VkViewport viewports{
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
			.x = 0,
			.y = 0,
			.width = (float)swapchain.displaySize_.width,
			.height = (float)swapchain.displaySize_.height,
	};

	VkRect2D scissor = {.extent = swapchain.displaySize_,
			.offset = {
					.x = 0, .y = 0,
			}};
	// Specify viewport info
	VkPipelineViewportStateCreateInfo viewportInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.viewportCount = 1,
			.pViewports = &viewports,
			.scissorCount = 1,
			.pScissors = &scissor,
	};

	// Specify multisample info
	VkSampleMask sampleMask = ~0u;
	VkPipelineMultisampleStateCreateInfo multisampleInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0,
			.pSampleMask = &sampleMask,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
	};

	// Specify color blend state
	VkPipelineColorBlendAttachmentState attachmentStates{
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			.blendEnable = VK_FALSE,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &attachmentStates,
			.flags = 0,
	};

	// Specify rasterizer info
	VkPipelineRasterizationStateCreateInfo rasterInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1,
	};

	// Specify input assembler state

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			// changed topology to vk_primitive_topology_triangle_strip to draw squares,
			// reverted to allow the vertex vector be be recreated from the board state
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
	};

	auto bindingDesc = Vertex::getBindingDesc();
	auto attributeDesc = Vertex::getAttributes();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDesc,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size()),
			.pVertexAttributeDescriptions = attributeDesc.data(),
	};

	// Create the pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			.pNext = nullptr,
			.initialDataSize = 0,
			.pInitialData = nullptr,
			.flags = 0,  // reserved, must be 0
	};

	CALL_VK(vkCreatePipelineCache(device.device_, &pipelineCacheInfo, nullptr,
								  &gfxPipeline.cache_));

	// Create the pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssemblyInfo,
			.pTessellationState = nullptr,
			.pViewportState = &viewportInfo,
			.pRasterizationState = &rasterInfo,
			.pMultisampleState = &multisampleInfo,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlendInfo,
			.pDynamicState = nullptr,
			.layout = gfxPipeline.layout_,
			.renderPass = render.renderPass_,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
	};

	VkResult pipelineResult = vkCreateGraphicsPipelines(
			device.device_, gfxPipeline.cache_, 1, &pipelineCreateInfo, nullptr,
			&gfxPipeline.pipeline_);

	// We don't need the shaders anymore, we can release their memory
	vkDestroyShaderModule(device.device_, vertexShader, nullptr);
	vkDestroyShaderModule(device.device_, fragmentShader, nullptr);

	return pipelineResult;
}

void DeleteGraphicsPipeline(void) {
	if (gfxPipeline.pipeline_ == VK_NULL_HANDLE) return;
	vkDestroyPipeline(device.device_, gfxPipeline.pipeline_, nullptr);
	vkDestroyPipelineCache(device.device_, gfxPipeline.cache_, nullptr);
	vkDestroyPipelineLayout(device.device_, gfxPipeline.layout_, nullptr);
}

void drawFrame(){

	for (int bufferIndex = 0; bufferIndex < swapchain.swapchainLength_; bufferIndex++) {
		// We start by creating and declare the "beginning" our command buffer
		VkCommandBufferBeginInfo cmdBufferBeginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr,
		};
		CALL_VK(vkBeginCommandBuffer(render.cmdBuffer_[bufferIndex], &cmdBufferBeginInfo));
		// transition the display image to color attachment layout
		setImageLayout(render.cmdBuffer_[bufferIndex],
					   swapchain.displayImages_[bufferIndex],
					   VK_IMAGE_LAYOUT_UNDEFINED,
					   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		// draw function

		// Now we start a renderpass. Any draw command has to be recorded in a
		// renderpass
		// TODO background color (really just the draw flush color and then the rest of the shapes are drawn over the draw flush)
		VkClearValue clearVals{
				.color.float32[0] = 0.15f,
				.color.float32[1] = 0.15f,
				.color.float32[2] = 0.15f,
				.color.float32[3] = 1.0f,
		};
		VkRenderPassBeginInfo renderPassBeginInfo{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = nullptr,
				.renderPass = render.renderPass_,
				.framebuffer = swapchain.framebuffers_[bufferIndex],
				.renderArea = {.offset =
						{
								.x = 0, .y = 0,
						},
						.extent = swapchain.displaySize_},
				.clearValueCount = 1,
				.pClearValues = &clearVals};

		vkCmdBeginRenderPass(render.cmdBuffer_[bufferIndex], &renderPassBeginInfo,
							 VK_SUBPASS_CONTENTS_INLINE);

		// Bind what is necessary to the command buffer
		vkCmdBindPipeline(render.cmdBuffer_[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
						  gfxPipeline.pipeline_);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(render.cmdBuffer_[bufferIndex], 0, 1, &buffers.vertexBuf_, &offset);

		/*  Draw Squares, the second parameter of vkCmdDraw is the number of vertices; needs to be
			using vk_primitive_topology_triangle_strip to draw squares (located on line 562) */
		// pass the size of the vector containing each vertex as the second argument
		vkCmdDraw(render.cmdBuffer_[bufferIndex],
				  static_cast<uint32_t>(vertices.size())/*numOfPieces * numOfFaces 6*/, 1, 0, 0);

		vkCmdEndRenderPass(render.cmdBuffer_[bufferIndex]);

		CALL_VK(vkEndCommandBuffer(render.cmdBuffer_[bufferIndex]));
	}
}

// InitVulkan:
//   Initialize Vulkan Context when android application window is created
//   upon return, vulkan is ready to draw frames
bool InitVulkan(android_app* app) {
	androidAppCtx = app;

	if (!InitVulkan()) {
		LOGW("Vulkan is unavailable, install vulkan and re-start");
		return false;
	}

	VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = nullptr,
			.apiVersion = VK_MAKE_VERSION(1, 0, 0),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.pApplicationName = "Tetris",
			.pEngineName = "Upgraded sample triangle tutorial",
	};

	// create a device
	CreateVulkanDevice(app->window, &appInfo);

	CreateSwapChain();

	// -----------------------------------------------------------------
	// Create render pass
	VkAttachmentDescription attachmentDescriptions{
			.format = swapchain.displayFormat_,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference colourReference = {
			.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

	VkSubpassDescription subpassDescription{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.flags = 0,
			.inputAttachmentCount = 0,
			.pInputAttachments = nullptr,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colourReference,
			.pResolveAttachments = nullptr,
			.pDepthStencilAttachment = nullptr,
			.preserveAttachmentCount = 0,
			.pPreserveAttachments = nullptr,
	};
	VkRenderPassCreateInfo renderPassCreateInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.attachmentCount = 1,
			.pAttachments = &attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 0,
			.pDependencies = nullptr,
	};
	CALL_VK(vkCreateRenderPass(device.device_, &renderPassCreateInfo, nullptr,
							   &render.renderPass_));

	// -----------------------------------------------
	// Create a pool of command buffers to allocate command buffer from
	VkCommandPoolCreateInfo cmdPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = device.queueFamilyIndex_,
	};
	CALL_VK(vkCreateCommandPool(device.device_, &cmdPoolCreateInfo, nullptr, &render.cmdPool_));

	// might be the start of the code that needs to be called each draw frame

	// Record a command buffer that just clear the screen
	// 1 command buffer draw in 1 framebuffer
	// In our case we need 2 command as we have 2 framebuffer
	render.cmdBufferLen_ = swapchain.swapchainLength_;
	render.cmdBuffer_ = new VkCommandBuffer[swapchain.swapchainLength_];
	VkCommandBufferAllocateInfo cmdBufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = render.cmdPool_,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = render.cmdBufferLen_,
	};
	CALL_VK(vkAllocateCommandBuffers(device.device_, &cmdBufferCreateInfo, render.cmdBuffer_));

	// -----------------------------------------------------------------
	// Create 2 frame buffers.
	CreateFrameBuffers(render.renderPass_);

	//TODO update the values passed every draw
	CreateBuffers();  // create vertex buffers

	//CreateBuffers(0,0);
	// Create graphics pipeline
	CreateGraphicsPipeline();

	/*for (int bufferIndex = 0; bufferIndex < swapchain.swapchainLength_; bufferIndex++) {
		// We start by creating and declare the "beginning" our command buffer
		VkCommandBufferBeginInfo cmdBufferBeginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr,
		};
		CALL_VK(vkBeginCommandBuffer(render.cmdBuffer_[bufferIndex], &cmdBufferBeginInfo));
		// transition the display image to color attachment layout
		setImageLayout(render.cmdBuffer_[bufferIndex],
					   swapchain.displayImages_[bufferIndex],
					   VK_IMAGE_LAYOUT_UNDEFINED,
					   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
*/
		// draw function
		drawFrame();
	//}

	// We need to create a fence to be able, in the main loop, to wait for our
	// draw command(s) to finish before swapping the framebuffers
	VkFenceCreateInfo fenceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
	};
	CALL_VK(vkCreateFence(device.device_, &fenceCreateInfo, nullptr, &render.fence_));

	// We need to create a semaphore to be able to wait, in the main loop, for our
	// framebuffer to be available for us before drawing.
	VkSemaphoreCreateInfo semaphoreCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
	};
	CALL_VK(vkCreateSemaphore(device.device_, &semaphoreCreateInfo, nullptr, &render.semaphore_));
	device.initialized_ = true;
	return true;
}

//TODO draw the game board state and current piece with their respective colors

//TODO write rotation code for each piece

//TODO write bounding box code for each piece

// IsVulkanReady():
//    native app poll to see if we are ready to draw...
bool IsVulkanReady(void) {
	/*for(int i = 0; i < 20; i++){
		gameboard[i][0] = rand() % 7 + 1;
		//gameboard[19][i] = rand() % 7 + 1;
		rowSize[i] = 1;
	}
	//rowSize[0] = rowSize[19] = 10;*/
	return device.initialized_;
}

void DeleteVulkan(void) {
	vkFreeCommandBuffers(device.device_, render.cmdPool_, render.cmdBufferLen_, render.cmdBuffer_);
	delete[] render.cmdBuffer_;

	vkDestroyCommandPool(device.device_, render.cmdPool_, nullptr);
	vkDestroyRenderPass(device.device_, render.renderPass_, nullptr);
	DeleteSwapChain();
	DeleteGraphicsPipeline();
	DeleteBuffers();

	vkDestroyDevice(device.device_, nullptr);
	vkDestroyInstance(device.instance_, nullptr);

	device.initialized_ = false;
}

// Draw one frame
bool VulkanDrawFrame(void) {
	uint32_t nextIndex;
	// Get the framebuffer index we should draw in

	// update the vertex buffer then, bind the vertex data, and draw the frame to the screen
	CreateBuffers();
	drawFrame();


	CALL_VK(vkAcquireNextImageKHR(device.device_, swapchain.swapchain_, UINT64_MAX, render.semaphore_, VK_NULL_HANDLE, &nextIndex));
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &render.semaphore_,
			.pWaitDstStageMask = &waitStageMask,
			.commandBufferCount = 1,
			.pCommandBuffers = &render.cmdBuffer_[nextIndex],
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr};
	CALL_VK(vkQueueSubmit(device.queue_, 1, &submit_info, render.fence_));
	CALL_VK(vkWaitForFences(device.device_, 1, &render.fence_, VK_TRUE, 100000000));


	//drawFrame(nextIndex);

	//LOGI("Drawing frames......");
	//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " current piece y %d", currentPieceY);

	VkResult result;
	VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.swapchainCount = 1,
			.pSwapchains = &swapchain.swapchain_,
			.pImageIndices = &nextIndex,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pResults = &result,
	};
	vkQueuePresentKHR(device.queue_, &presentInfo);
	return true;
}

/*
 * setImageLayout():
 *    Helper function to transition color buffer layout
 */
void setImageLayout(VkCommandBuffer cmdBuffer, VkImage image,
					VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
					VkPipelineStageFlags srcStages,
					VkPipelineStageFlags destStages) {
	VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = oldImageLayout,
			.newLayout = newImageLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange =
					{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
					},
	};

	switch (oldImageLayout) {
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		default:
			break;
	}

	switch (newImageLayout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			imageMemoryBarrier.dstAccessMask =
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			break;

		default:
			break;
	}

	vkCmdPipelineBarrier(cmdBuffer, srcStages, destStages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
}
