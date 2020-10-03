#include "Omnia/Omnia.pch"
#include "GuiLayer.h"

#include "Omnia/Core/Application.h"
#include "Omnia/Log.h"

#include "Platform/GFX/Vulkan/VKContext.h"

#include "Omnia/UI/GuiBuilder.h"

namespace Omnia {

static bool ShowDemoWindow = false;

static void check_vk_result(VkResult err) {
    if (err == 0) return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) abort();
}

static void FrameRender(ImDrawData* draw_data)
{
    VkResult err;
    VKContext *context = &reinterpret_cast<VKContext &>(Application::Get().GetContext());



    /// @brief 
    /// @param draw_data 
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    //VkBuffer vertexBuffers[] = { vertexBuffer };
    //VkDeviceSize offsets[] = { 0 };

    //uint32_t imageIndex;
    //VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    //VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;

    //err = vkAcquireNextImageKHR(data->iDevice->GetDevice(), wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    //if (err == VK_ERROR_OUT_OF_DATE_KHR)
    //{
    //    g_SwapChainRebuild = true;
    //    return;
    //}
    //check_vk_result(err);

    //ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    //{
    //    err = vkWaitForFences(data->iDevice->GetDevice(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
    //    check_vk_result(err);

    //    err = vkResetFences(data->iDevice->GetDevice(), 1, &fd->Fence);
    //    check_vk_result(err);
    //}
    //{
    //    err = vkResetCommandPool(data->iDevice->GetDevice(), fd->CommandPool, 0);
    //    check_vk_result(err);
    //    VkCommandBufferBeginInfo info = {};
    //    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    //    check_vk_result(err);
    //}

    // Record dear imgui primitives into command buffer
    //context->GetSwapChain()->Prepare();
    //auto buffer = context->GetDevice()->GetCommandBuffer(true);
    //ImGui_ImplVulkan_RenderDrawData(draw_data, buffer);
    //context->GetDevice()->FlushCommandBuffer(buffer);
    //context->GetSwapChain()->QueuePresent(context->GetDevice()->GetQueue(), context->GetSwapChain()->GetCurrentBufferIndex(), nullptr);

    // Submit command buffer
    //vkCmdEndRenderPass(fd->CommandBuffer);
    //{
    //    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //    VkSubmitInfo info = {};
    //    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    info.waitSemaphoreCount = 1;
    //    info.pWaitSemaphores = &image_acquired_semaphore;
    //    info.pWaitDstStageMask = &wait_stage;
    //    info.commandBufferCount = 1;
    //    info.pCommandBuffers = &fd->CommandBuffer;
    //    info.signalSemaphoreCount = 1;
    //    info.pSignalSemaphores = &render_complete_semaphore;

    //    err = vkEndCommandBuffer(fd->CommandBuffer);
    //    check_vk_result(err);
    //    err = vkQueueSubmit(data->Queue, 1, &info, fd->Fence);
    //    check_vk_result(err);
    //}
}

static void FramePresent()
{
    Application &app = Application::Get();
    //if (g_SwapChainRebuild) return;
    //VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    //VkPresentInfoKHR info = {};
    //info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //info.waitSemaphoreCount = 1;
    //info.pWaitSemaphores = &render_complete_semaphore;
    //info.swapchainCount = 1;
    //info.pSwapchains = &wd->Swapchain;
    //info.pImageIndices = &wd->FrameIndex;
    //VkResult err = vkQueuePresentKHR(data->Queue, &info);
    //if (err == VK_ERROR_OUT_OF_DATE_KHR)
    //{
    //    g_SwapChainRebuild = true;
    //    return;
    //}
    //check_vk_result(err);
    //wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores

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
    if (Context::API != GraphicsAPI::Vulkan) {
    	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    }
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
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
        VKContext *context = &reinterpret_cast<VKContext &>(Application::Get().GetContext());
        ImGui_ImplWin32_Init(app.GetWindow().GetNativeWindow(), nullptr);

        // Create Descriptor Pool
        // UI Descriptor Pool
        VkDescriptorPool descriptorPool;
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
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VkResult err = vkCreateDescriptorPool(context->GetDevice()->Call(), &pool_info, nullptr, &descriptorPool);
        check_vk_result(err);

        // UI Command Buffer/Pool, Framebuffers, RenderPass
        ImGui_ImplVulkan_InitInfo vkInfo = {};
        vkInfo.CheckVkResultFn = check_vk_result;
        vkInfo.Instance = context->GetInstance()->Call();
        vkInfo.PhysicalDevice = context->GetPhyiscalDevice()->Call();
        vkInfo.Device = context->GetDevice()->Call();
        vkInfo.QueueFamily = context->GetPhyiscalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
        vkInfo.Queue = context->GetDevice()->GetQueue();
        vkInfo.DescriptorPool = descriptorPool;
        vkInfo.MinImageCount = context->GetSwapChain()->GetImageCount();
        vkInfo.ImageCount = context->GetSwapChain()->GetImageCount();
        ImGui_ImplVulkan_Init(&vkInfo, context->GetRenderPass());
        // Upload Fonts
        {
            // Use any command queue
            vk::CommandBuffer commandBuffer =  context->GetDevice()->GetCommandBuffer(true);
            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
            context->GetDevice()->FlushCommandBuffer(commandBuffer);

            vkDeviceWaitIdle(context->GetDevice()->Call());
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
     }
}

void GuiLayer::Detach() {
    Application &app = Application::Get();
    if (Context::API == GraphicsAPI::OpenGL) ImGui_ImplOpenGL3_Shutdown();
    if (Context::API == GraphicsAPI::Vulkan) {
        //vkDeviceWaitIdle(context->GetDevice()->Call());
        ImGui_ImplVulkan_Shutdown();
    }
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
        FramePresent();
        FrameRender(ImGui::GetDrawData());
    }

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
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

