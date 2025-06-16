#include "UIManager.h"

#include "Renderer.h"

#include <functional>
#include <queue>
#include <string.h>

typedef std::function<void()> VoidFunc;
typedef std::pair<VoidFunc, VoidFunc> VoidFuncPair;

UIManager::UIManager()
    : m_NextViewportHandle(-1)
{
    //SaveSettings();
}

UIManager::~UIManager()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void UIManager::InitStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Corners
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.48f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.37f, 0.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.04f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.16f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.26f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.00f, 1.00f, 0.40f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.48f, 0.26f, 0.98f, 0.52f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.06f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.26f, 0.98f, 0.50f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.58f, 0.73f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.29f, 0.20f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ImGuiIO& io = ImGui::GetIO();
    m_DefaultFont = io.Fonts->AddFontFromFileTTF("include\\imgui\\misc\\fonts\\Roboto-Medium.ttf", 13);
    io.Fonts->Build();
}

void UIManager::InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount, IRenderSettings* renderer)
{
    assert(device);
    assert(commandQueue);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = device;
    init_info.CommandQueue = commandQueue;
    init_info.NumFramesInFlight = swapchainBufferCount;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.

    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // The example_win32_directx12/main.cpp application include a simple free-list based allocator.
    init_info.SrvDescriptorHeap = descriptorHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return Renderer::ImGuiAllocateDescriptor(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return Renderer::ImGuiFreeDescriptor(cpu_handle, gpu_handle); };

    ImGui_ImplDX12_Init(&init_info);


    m_Renderer = renderer;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
}

void UIManager::Render()
{
    if (m_UISettings.m_DockSpace.GetValue()) ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (m_ActiveWindows.m_ShowDemoWindow) ImGui::ShowDemoWindow(&m_ActiveWindows.m_ShowDemoWindow);    
    
    MainMenuBar();       
    SettingsWindow();
    DrawViewports();
}
 
void UIManager::DrawViewports()
{
    for (auto& viewport : m_Viewports)
    {
        ViewportHandle handle = viewport.first;
        Viewport& vp = viewport.second;
        ViewportTextureHandle& vpTexHandle = vp.m_TextureHandle;
        ViewportTexture& vpTex = m_ViewportDisplayTextureHandles[vpTexHandle];

        // Begin the window
        std::string viewportName = "Viewport##" + std::to_string(handle);
        ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(viewportName.c_str(), &vp.m_Open, ImGuiWindowFlags_NoScrollbar))
        {
            float cursorY = ImGui::GetCursorPosY();

            if (vpTexHandle != 0)
            {
                // Calculate the image size and position
                ImVec2 windowDims = ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - cursorY);
                ImVec2 texDims = ImVec2(vpTex.m_TextureWidth, vpTex.m_TextureHeight);
                ImVec2 textureSize = CalculateViewportTextureSize(texDims, windowDims);
                ImVec2 texturePosition = ImVec2(windowDims.x > windowDims.y ? windowDims.x / 2.0f - textureSize.x / 2.0f : 0.0f, windowDims.y / 2.0f - textureSize.y / 2.0f);
                texturePosition.y += cursorY;

                // Draw the view
                ImGui::SetCursorPos(texturePosition);
                ImGui::Image(TexHandleToImTexID(vpTex.m_TextureHandle), textureSize);
            }
            // Draw the selection Combo Box
            ImGui::SetCursorPosY(cursorY);
            ImGui::PushItemWidth(250.f);
            if (ImGui::BeginCombo("View", vpTex.m_DebugName.c_str()))
            {
                u32 id = 0;
                ViewportTextureHandle currentSelection = vpTexHandle;
                for (auto it : m_ViewportDisplayTextureHandles)
                {
                    const char* debugName = it.second.m_DebugName.c_str();
                    bool isSelected = it.first == currentSelection;
                    ImGui::PushID(id++);
                    if (ImGui::Selectable(debugName, isSelected))
                    {
                        vpTexHandle = it.first;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
        ImGui::End();
    }
}

void UIManager::DestroyClosedViewports()
{
    std::queue<ViewportHandle> viewportsToDestroy;
    for (auto& it : m_Viewports)
    {
        if (!it.second.m_Open)
        {
            viewportsToDestroy.emplace(it.first);
        }
    }

    while (viewportsToDestroy.size() > 0)
    {
        ViewportHandle handle = viewportsToDestroy.front();
        viewportsToDestroy.pop();
        m_Viewports.erase(handle);
    }
}

void UIManager::SubmitViewportTexture(std::string textureName, GPUTextureHandle textureHandle, u32 textureWidth, u32 textureHeight)
{
    ViewportTextureHandle handle = GetViewportTextureHandle(textureName);
    assert(m_ViewportDisplayTextureHandles.find(handle) == m_ViewportDisplayTextureHandles.end());
    if (m_ViewportDisplayTextureHandles.find(handle) == m_ViewportDisplayTextureHandles.end())
    {
        ViewportTexture vp;
        vp.m_TextureHandle = textureHandle;
        vp.m_TextureWidth = textureWidth;
        vp.m_TextureHeight = textureHeight;
        vp.m_DebugName = textureName;
        m_ViewportDisplayTextureHandles[handle] = vp;
    }
    else
    {
        assert(false);
    }
}

void UIManager::CreateViewport()
{
    m_Viewports[AllocateViewportHandle()] = { GetViewportTextureHandle(GetDefaultViewName()), true }; // default open to true
}

ViewportHandle UIManager::AllocateViewportHandle()
{
    u32 handle = ++m_NextViewportHandle;
    return handle;
}

std::string UIManager::GetDefaultViewName()
{
    return "Scene";
}

void UIManager::BeginRender()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(m_DefaultFont);
}

void UIManager::EndRender(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer)
{
    ImGui::PopFont();
    ImGui::Render();
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmdList->ResourceBarrier(1, &barrier);

    CleanUp();
}

void UIManager::CleanUp()
{
    m_ViewportDisplayTextureHandles.clear();

    DestroyClosedViewports();
}

void UIManager::MainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // FILE
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("(demo menu)", NULL, false, false);
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::BeginMenu("Open Recent"))
            {
                ImGui::MenuItem("fish_hat.c");
                ImGui::MenuItem("fish_hat.inl");
                ImGui::MenuItem("fish_hat.h");
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            ImGui::EndMenu();
        }

        // EDIT
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) m_ActiveWindows.m_SettingsWindow = !m_ActiveWindows.m_SettingsWindow;
            ImGui::EndMenu();
        }

        // WINDOW
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Add Viewport")) CreateViewport();
            if (ImGui::MenuItem("Dockspace"))
            {
                m_UISettings.m_DockSpace.m_Value = !m_UISettings.m_DockSpace.m_Value;
                m_Renderer->SetRenderToMainRTV(m_UISettings.m_DockSpace.m_Value);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Show Demo")) m_ActiveWindows.m_ShowDemoWindow = !m_ActiveWindows.m_ShowDemoWindow;
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
}

void UIManager::SettingsWindow()
{
    if (m_ActiveWindows.m_SettingsWindow)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Settings", &m_ActiveWindows.m_SettingsWindow, ImGuiWindowFlags_MenuBar))
        {
            std::unordered_map<std::string, std::function<void(UIManager&)>> settingsPageMap =
            {
                { "Scene", &UIManager::SceneSettingsPage }
            };

            // Left
            static std::string selected = settingsPageMap.size() > 0 ? settingsPageMap.begin()->first : "";
            {
                ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
                // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
                for (auto& it : settingsPageMap)
                {
                    std::string categoryName = it.first;
                    if (ImGui::Selectable(categoryName.c_str(), selected == categoryName))
                    {
                        selected = categoryName;
                    }
                }
                ImGui::EndChild();
            }
            ImGui::SameLine();

            // Right
            // call settings page function
            ImGui::BeginGroup();
            ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
            settingsPageMap[selected](*this);
            ImGui::EndChild();
            ImGui::EndGroup();
        }
        ImGui::End();
    }
}

void UIManager::SceneSettingsPage()
{
    RenderSettings& renderSettings = m_Renderer->GetRenderSettings();
    std::vector<VoidFuncPair> settingsDisplayFunctions;

    VoidFuncPair mainRtvColour =
    {
        [&]() { ImGui::Text(renderSettings.m_MainViewportClearColour.GetName()); },
        [&]() { ImGui::ColorEdit4(renderSettings.m_MainViewportClearColour.GetLabelessName().c_str(), (float*)&renderSettings .m_MainViewportClearColour.m_Value, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None); }
    };
    settingsDisplayFunctions.push_back(mainRtvColour);

    VoidFuncPair dockSpace =
    {
        [&]() { ImGui::Text(m_UISettings.m_DockSpace.GetName()); },
        [&]() { if (ImGui::Checkbox(m_UISettings.m_DockSpace.GetLabelessName().c_str(), &m_UISettings.m_DockSpace.m_Value)) m_Renderer->SetRenderToMainRTV(m_UISettings.m_DockSpace.m_Value); }
    };
    settingsDisplayFunctions.push_back(dockSpace);
    

    // left
    ImGui::BeginGroup();
    for (auto& it : settingsDisplayFunctions)
    {
        it.first();
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    // right
    ImGui::BeginGroup();
    for (auto& it : settingsDisplayFunctions)
    {
        it.second();
    }
    ImGui::EndGroup();

    
}

void UIManager::SaveSettings()
{

}

ViewportTextureHandle UIManager::GetViewportTextureHandle(std::string debugName)
{
    return std::hash<std::string>{}(debugName);
}

ImVec2 UIManager::CalculateViewportTextureSize(ImVec2& const textureDims, ImVec2& const windowDims)
{
    // resize the texture to allways fit the bounds of the viewport
    float width = windowDims.x;
    float windowAspect = windowDims.y / width;
    float texAspect = textureDims.y / textureDims.x;
    float aspectDiff = windowAspect / texAspect;
    float imageHeight = texAspect < windowAspect ? width * texAspect : width * texAspect * aspectDiff;
    float imageWidth = texAspect < windowAspect ? width : width * aspectDiff;
    return ImVec2(imageWidth, imageHeight);
}

ImTextureID UIManager::TexHandleToImTexID(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    // convert to ImU64 pointer, and dereference
    D3D12_GPU_DESCRIPTOR_HANDLE* handlePtr = &(handle);
    ImTextureID* ITID = (ImTextureID*)handlePtr;
    return *ITID;
}
