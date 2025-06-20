#pragma once
#include "EngineCore.h"
#include "EngineUtils.h"

#include "Settings.h"
#include "RenderSettings.h"

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

typedef u32 ViewportHandle;
typedef u64 ViewportTextureHandle;

PROPERTY_CONFIG_BEGIN(UISettings)
	PROPERTY(bool, DockSpace, false)
PROPERTY_CONFIG_END

struct Viewport
{
	ViewportTextureHandle m_TextureHandle;
	bool m_Open;
};

struct ViewportTexture
{
	u32 m_TextureWidth;
	u32 m_TextureHeight;
	GPUTextureHandle m_TextureHandle;
	std::string m_DebugName;
};

class UIManager
{
public:
	UIManager();
	~UIManager();

	struct ActiveWindows
	{
		ActiveWindows()
			: m_ShowDemoWindow(false)
			, m_SettingsWindow(false)
		{
		}

		bool m_SettingsWindow;
		bool m_ShowDemoWindow;
	};

	void InitStyle();

	void InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount, IRenderSettings* renderer);

	void BeginRender();
	void Render();
	void EndRender(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);

	void SubmitViewportTexture(std::string textureName, GPUTextureHandle textureHandle, u32 textureWidth, u32 textureHeight);
	void CreateViewport();

	std::string GetDefaultViewName();

private:

	IRenderSettings* m_Renderer;

	UISettings m_UISettings;

	void CleanUp();
	// UI Draw functions
	void DrawViewports();
	void DestroyClosedViewports();
	void MainMenuBar();
	void SettingsWindow();

	// Settings Categories
	void SceneSettingsPage();

	// XML
	void SaveSettings();

	// Utility Functions
	ViewportTextureHandle GetViewportTextureHandle(std::string debugName);
	ImVec2 CalculateViewportTextureSize(ImVec2& const textureDims, ImVec2& const windowDims);
	ImTextureID TexHandleToImTexID(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	ViewportHandle AllocateViewportHandle();

	ActiveWindows m_ActiveWindows;

	std::unordered_map<ViewportHandle, Viewport> m_Viewports;
	std::unordered_map<ViewportTextureHandle, ViewportTexture> m_ViewportDisplayTextureHandles;
	ViewportHandle m_NextViewportHandle;

	ImVec4* m_DefaultUIColours;
	ImFont* m_DefaultFont;
};

