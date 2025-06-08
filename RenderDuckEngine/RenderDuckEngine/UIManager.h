#pragma once
#include "EngineCore.h"

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

#include <cstring>
#include <string>
#include <unordered_map>


typedef u32 ViewportHandle;
typedef u64 ViewportTextureHandle;

struct ViewportTexture
{
	u32 m_TextureWidth;
	u32 m_TextureHeight;
	GPUTextureHandle m_TextureHandle;
	std::string m_DebugName;
};

struct ConfigParams
{
	ConfigParams()
		: m_ShowDemoWindow(false)
		, m_DockSpace(false)
	{
	}

	bool m_ShowDemoWindow;
	bool m_DockSpace;
};

class UIManager
{
public:
	UIManager();
	~UIManager();

	void InitStyle();

	void InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount);

	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);

	void SubmitViewportTexture(std::string textureName, GPUTextureHandle textureHandle, u32 textureWidth, u32 textureHeight);
	void CreateViewport();

	const ConfigParams GetParams();

	std::string GetDefaultViewName();

private:

	void BeginRender();
	void EndRender(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);
	void CleanUp();
	// UI Draw functions
	void DrawViewports();
	void MainMenuBar();

	// Utility Functions
	ViewportTextureHandle GetViewportTextureHandle(std::string debugName);
	ImVec2 CalculateViewportTextureSize(ImVec2& const textureDims, ImVec2& const windowDims);
	ImTextureID TexHandleToImTexID(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	ViewportHandle AllocateViewportHandle();

	ConfigParams m_Params;

	std::unordered_map<ViewportHandle, ViewportTextureHandle> m_Viewports;
	std::unordered_map<ViewportTextureHandle, ViewportTexture> m_ViewportDisplayTextureHandles;
	ViewportHandle m_NextHandle;

	ImVec4* m_DefaultUIColours;
	ImGuiStyle m_DefaultUIStyle;
	ImFont* m_DefaultFont;
};

