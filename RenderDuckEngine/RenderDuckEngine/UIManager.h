#pragma once
#include "EngineCore.h"
#include "EngineUtils.h"

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

#include <cstring>
#include <string>

typedef u32 ViewportHandle;
typedef u64 ViewportTextureHandle;

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

class UIManager;
template<typename T>
class SettingMap
{
public:

	friend UIManager;

	SettingMap(std::string name, T value)
		: m_Value(value), m_Name(name)
	{
	}

	const char* GetName() { return m_Name.c_str(); }
	const std::string GetLabelessName() { return "###" + m_Name; }
	const T& GetValue() const { return m_Value; }

private:

	T m_Value;
	std::string m_Name;
};

class UIManager
{
public:
	UIManager();
	~UIManager();

	struct Settings
	{
		Settings() 
			: m_DockSpace("DockSpace", false)
			, m_MainViewportClearColour("MainRTVClearColour", DirectXColorToImVec4(DirectX::Colors::DimGray))
		{
		}

		SettingMap<ImVec4> m_MainViewportClearColour;

		SettingMap<bool> m_DockSpace;
	};

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

	void InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount);

	void BeginRender();
	void Render();
	void EndRender(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);

	void SubmitViewportTexture(std::string textureName, GPUTextureHandle textureHandle, u32 textureWidth, u32 textureHeight);
	void CreateViewport();

	const Settings& GetSettings() { return m_Settings; }

	std::string GetDefaultViewName();

private:
	void CleanUp();
	// UI Draw functions
	void DrawViewports();
	void DestroyClosedViewports();
	void MainMenuBar();
	void SettingsWindow();

	// Settings Categories
	void SceneSettingsPage();

	// Utility Functions
	ViewportTextureHandle GetViewportTextureHandle(std::string debugName);
	ImVec2 CalculateViewportTextureSize(ImVec2& const textureDims, ImVec2& const windowDims);
	ImTextureID TexHandleToImTexID(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	ViewportHandle AllocateViewportHandle();

	Settings m_Settings;
	ActiveWindows m_ActiveWindows;

	std::unordered_map<ViewportHandle, Viewport> m_Viewports;
	std::unordered_map<ViewportTextureHandle, ViewportTexture> m_ViewportDisplayTextureHandles;
	ViewportHandle m_NextHandle;

	ImVec4* m_DefaultUIColours;
	ImGuiStyle m_DefaultUIStyle;
	ImFont* m_DefaultFont;
};

