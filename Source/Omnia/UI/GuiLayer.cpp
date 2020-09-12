#include "Omnia/Omnia.pch"
#include "GuiLayer.h"

#include "Omnia/Core/Application.h"
#include "Omnia/Log.h"

#include "Platform/GFX/Vulkan/VKContext.h"

#include "Omnia/UI/GuiBuilder.h"

namespace Omnia {

static bool ShowDemoWindow = false;

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = {};
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Could not create one-time command buffer!");
    }

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, VkCommandPool cmdPool) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}


VkCommandPool uiCommandPool;
vector<VkCommandBuffer> uiCommandBuffers;
VkDescriptorPool uiDescriptorPool;
VkRenderPass uiRenderPass;
vector<VkFramebuffer> uiFramebuffers;
VkFormat swapchainImageFormat;
vector<VkImage> swapchainImages;
vector<VkImageView> swapchainImageViews;
VkExtent2D swapchainExtent;

void createUICommandBuffers(VkDevice device) {
    uiCommandBuffers.resize(swapchainImageViews.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = uiCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(uiCommandBuffers.size());

    if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, uiCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Unable to allocate UI command buffers!");
    }
}

void createUICommandPool(VkDevice device, uint32_t queueIndex, VkCommandPool *cmdPool, VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = queueIndex;
    commandPoolCreateInfo.flags = flags;

    if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, cmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create graphics command pool!");
    }
}

void createUIDescriptorPool(VkDevice device) {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &uiDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Cannot allocate UI descriptor pool!");
    }
}

void createUIFramebuffers(VkDevice device) {
    // Create some UI framebuffers. These will be used in the render pass for the UI
    uiFramebuffers.resize(swapchainImages.size());
    VkImageView attachment[1];
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = uiRenderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = swapchainExtent.width;
    info.height = swapchainExtent.height;
    info.layers = 1;
    for (uint32_t i = 0; i < swapchainImages.size(); ++i) {
        attachment[0] = swapchainImageViews[i];
        if (vkCreateFramebuffer(device, &info, nullptr, &uiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Unable to create UI framebuffers!");
        }
    }
}

void createUIRenderPass(VkDevice device) {
    // Create an attachment description for the render pass
    VkAttachmentDescription attachmentDescription = {};
    attachmentDescription.format = swapchainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Need UI to be drawn on top of main
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Last pass so we want to present after
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Create a color attachment reference
    VkAttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create a subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    // Create a subpass dependency to synchronize our main and UI render passes
    // We want to render the UI after the geometry has been written to the framebuffer
    // so we need to configure a subpass dependency as such
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Create external dependency
    subpassDependency.dstSubpass = 0; // The geometry subpass comes first
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Wait on writes
    subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Finally create the UI render pass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &subpassDependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &uiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create UI render pass!");
    }
}



GuiLayer::GuiLayer(): Layer("GuiLayer") {}
GuiLayer::~GuiLayer() {}

void GuiLayer::Attach() {
	// Decide GL+GLSL versions
	Application &app = Application::Get();
	//ImGui_ImplWin32_EnableDpiAwareness();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
//	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.ConfigDockingWithShift = false;

	// ToDo: Works only as an memory leak, the question is why (otherwise ImGui uses old pointer where the data is gone) ...
	string *dataTarget = new string("./Data/"s + Application::GetWindow().GetProperties().Title + ".ini"s);
	string *logTarget = new string("./Log/"s + Application::GetWindow().GetProperties().Title + ".log"s);
	io.IniFilename = dataTarget->c_str();
	io.LogFilename = logTarget->c_str();
	//io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	//io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.DisplaySize = ImVec2((float)app.GetWindow().GetContexttSize().Width, (float)app.GetWindow().GetContexttSize().Height);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

    // Load Fonts
    io.Fonts->AddFontFromFileTTF("./Assets/Fonts/Roboto/Roboto-Medium.ttf", 14.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 14.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 14.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontDefault();

    if (Context::API == GraphicsAPI::OpenGL) {
        const char *glsl_version = "#version 450";
        ImGui_ImplWin32_Init(app.GetWindow().GetNativeWindow(), app.GetContext().GetNativeContext());
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
    if (Context::API == GraphicsAPI::Vulkan) {
        Application &app = Application::Get();
        VkContextData *data = (VkContextData*)(app.GetContext().GetNativeContext());

        // Initialize some DearImgui specific resources
        createUIDescriptorPool(data->Device);
        createUIRenderPass(data->Device);
        createUICommandPool(data->Device, data->QueueIndex, &uiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        createUICommandBuffers(data->Device);
        createUIFramebuffers(data->Device);

        ImGui_ImplWin32_Init(app.GetWindow().GetNativeWindow(), nullptr);

        static ImGui_ImplVulkan_InitInfo vkInfo;
        vkInfo.Instance = data->Intance;
        vkInfo.PhysicalDevice = data->PhysicalDevice;
        vkInfo.Device = data->Device;
        vkInfo.QueueFamily = data->QueueIndex;
        vkInfo.Queue = data->Queue;
        vkInfo.DescriptorPool = uiDescriptorPool;
        vkInfo.MinImageCount = data->ImageCount;
        vkInfo.ImageCount = data->ImageCount;
        vkInfo.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&vkInfo, uiRenderPass);

        VkCommandBuffer fontBuffer = beginSingleTimeCommands(data->Device, uiCommandPool);
        ImGui_ImplVulkan_CreateFontsTexture(fontBuffer);
        endSingleTimeCommands(data->Device, data->Queue, fontBuffer, uiCommandPool);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void GuiLayer::Detach() {
    if (Context::API == GraphicsAPI::OpenGL) ImGui_ImplOpenGL3_Shutdown();
    if (Context::API == GraphicsAPI::Vulkan) ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GuiLayer::GuiUpdate() {
	if (ShowDemoWindow) ImGui::ShowDemoWindow(&ShowDemoWindow);
}

void GuiLayer::Update(Timestamp deltaTime) {
	ImGuiIO &io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	return;
}


void GuiLayer::Prepare() {
	// Start new 'Dear ImGui' frame
    if (Context::API == GraphicsAPI::OpenGL) ImGui_ImplOpenGL3_NewFrame();
    if (Context::API == GraphicsAPI::Vulkan) {
        ImGui_ImplVulkan_NewFrame();
    }
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Properties
	ImGuiIO& io = ImGui::GetIO();
	static bool DockSpace = true;
	static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
	static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;;

	// Viewport
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) windowFlags |= ImGuiWindowFlags_NoBackground;

	// DockSpace
	ImGui::Begin("DockSpace", &DockSpace, windowFlags);
	ImGui::PopStyleVar(3);

	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}


}

void GuiLayer::Finish() {
	// ~DockSpace
	ImGui::End();

	// Properties
	Application &app = Application::Get();
	ImGuiIO& io = ImGui::GetIO();
	
	// Rendering
	ImGui::Render();
	io.DisplaySize = ImVec2((float)app.GetWindow().GetContexttSize().Width, (float)app.GetWindow().GetContexttSize().Height);
    if (Context::API == GraphicsAPI::OpenGL) ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (Context::API == GraphicsAPI::Vulkan) {
        VkContextData *data = (VkContextData*)(app.GetContext().GetNativeContext());
        uint32_t imageIndex = 1;
        VkResult result = vkAcquireNextImageKHR(data->Device, data->Swapchain, UINT64_MAX, data->Semaphore, VK_NULL_HANDLE, &imageIndex);
        VkCommandBufferBeginInfo cmdBufferBegin = {};
        cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // ToDo: Now its getting really a mess, maybe a fresh start would help more...
        //uint32_t bufferIdx = imageIndex;
        //if (vkBeginCommandBuffer(uiCommandBuffers[bufferIdx], &cmdBufferBegin) != VK_SUCCESS) {
        //    //throw std::runtime_error("Unable to start recording UI command buffer!");
        //}

        //VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        //VkRenderPassBeginInfo renderPassBeginInfo = {};
        //renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        //renderPassBeginInfo.renderPass = uiRenderPass;
        //renderPassBeginInfo.framebuffer = uiFramebuffers[bufferIdx];
        //renderPassBeginInfo.renderArea.extent.width = swapchainExtent.width;
        //renderPassBeginInfo.renderArea.extent.height = swapchainExtent.height;
        //renderPassBeginInfo.clearValueCount = 1;
        //renderPassBeginInfo.pClearValues = &clearColor;

        //vkCmdBeginRenderPass(uiCommandBuffers[bufferIdx], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


        //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), uiCommandBuffers);

        //vkCmdEndRenderPass(uiCommandBuffers[bufferIdx]);

        //if (vkEndCommandBuffer(uiCommandBuffers[bufferIdx]) != VK_SUCCESS) {
        //    throw std::runtime_error("Failed to record command buffers!");
        //}

    }

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		//app.Get().GetContext().Attach();
	}
}


void GuiLayer::Event(void *event) {}
void GuiLayer::ControllerEvent(ControllerEventData data) {}

void GuiLayer::KeyboardEvent(KeyboardEventData data) {
	if (ImGui::GetCurrentContext() == NULL) return;

	ImGuiIO &io = ImGui::GetIO();

	io.KeyAlt = data.Modifier.Alt;
	io.KeyCtrl = data.Modifier.Control;
	io.KeyShift = data.Modifier.Shift;
	io.KeySuper = data.Modifier.Super;

	//case KeyCode::F1: {
	//	//Menu::show_demo_window = !Menu::show_demo_window;
	//	break;
	//}
	switch (data.Action) {
		case KeyboardAction::Input: {
			io.AddInputCharacterUTF16((uint32_t)data.Key);
			break;
		}
		case KeyboardAction::Default: {
			switch (data.State) {
				case KeyState::Press: {
					io.KeysDown[(uint32_t)data.Key] = true;

					if (data.Key == KeyCode::F1) ShowDemoWindow = true;

					break;
				}
				case KeyState::Release: {
					io.KeysDown[(uint32_t)data.Key] = false;
					break;
				}

				default: {
					break;
				}
			}
		}
		default: {
			break;
		}
	}
}

void GuiLayer::MouseEvent(MouseEventData data) {
	if (ImGui::GetCurrentContext() == NULL) return;
	ImGuiIO &io = ImGui::GetIO();

	io.KeyAlt = data.Modifier.Alt;
	io.KeyCtrl = data.Modifier.Control;
	io.KeyShift = data.Modifier.Shift;
	io.KeySuper = data.Modifier.Super;

	switch (data.Action) {
		case MouseAction::Move:	{
			io.MousePos = ImVec2(static_cast<float>(data.X), static_cast<float>(data.Y));
		}

		case MouseAction::Wheel: {
			io.MouseWheel += data.DeltaWheelY;
			io.MouseWheelH += data.DeltaWheelX;
			break;
		}

		default: {
			switch (data.State) {
				case ButtonState::Press: {
					if (data.Button == MouseButton::Left)	io.MouseDown[0] = true;
					if (data.Button == MouseButton::Right)	io.MouseDown[1] = true;
					if (data.Button == MouseButton::Middle)	io.MouseDown[2] = true;
					if (data.Button == MouseButton::X1)		io.MouseDown[3] = true;
					if (data.Button == MouseButton::X2)		io.MouseDown[4] = true;
					break;
				}

				case ButtonState::Release: {
					if (data.Button == MouseButton::Left)	io.MouseDown[0] = false;
					if (data.Button == MouseButton::Right)	io.MouseDown[1] = false;
					if (data.Button == MouseButton::Middle) io.MouseDown[2] = false;
					if (data.Button == MouseButton::X1)		io.MouseDown[3] = false;
					if (data.Button == MouseButton::X2)		io.MouseDown[4] = false;
					break;
				}

				default: {
					break;
				}
			}
		}
	}
}

void GuiLayer::TouchEvent(TouchEventData data) {}
void GuiLayer::WindowEvent(WindowEventData data) {}


// Helpers
namespace UI {

}

}

