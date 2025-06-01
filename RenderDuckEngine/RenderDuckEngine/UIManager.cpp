#include "UIManager.h"

UIManager::UIManager()
{
}

UIManager::~UIManager()
{
}

//void UIManager::InitGUI(HWND wnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT swapchainBufferCount)
//{
//    // Setup Dear ImGui context
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO();
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
//
//    // Setup Platform/Renderer backends
//    ImGui_ImplWin32_Init(wnd);
//    ImGui_ImplDX12_InitInfo init_info = {};
//    init_info.Device = device;
//    init_info.CommandQueue = commandQueue;
//    init_info.NumFramesInFlight = swapchainBufferCount;
//    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.
//
//    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
//    // The example_win32_directx12/main.cpp application include a simple free-list based allocator.
//    init_info.SrvDescriptorHeap = m_SrvDescriptorHeap.Get();
//    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return s_DescriptorHeapAllocator.Alloc(out_cpu_handle, out_gpu_handle); };
//    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return s_DescriptorHeapAllocator.Free(cpu_handle, gpu_handle); };
//
//    ImGui_ImplDX12_Init(&init_info);
//
//    // Setup Dear ImGui style
//    ImGui::StyleColorsDark();
//    //ImGui::StyleColorsLight();
//}

void UIManager::DrawGUI()
{
}
