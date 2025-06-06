#pragma once
#include "EngineCore.h"

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

#include <unordered_map>

struct ImGuiParams
{
	ImGuiParams() :
		m_ShowDemoWindow(false)
	{ }

	bool m_ShowDemoWindow;
	bool m_DockSpace;
};

typedef D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle;
typedef u32 ViewportHandle;

class UIManager
{
public:
	UIManager();
	~UIManager();

	struct  Viewport
	{
		Viewport() {};
		Viewport(TextureHandle texHandle, float texWidth, float texHeight)
			: m_TextureHandle(texHandle)
		{
			m_Dimensions.m_TextureHeight = texHeight;
			m_Dimensions.m_TextureWidth = texWidth;
		}

		struct Dimensions
		{
			float m_TextureWidth;
			float m_TextureHeight;
			float m_WindowWidth;
			float m_WindowHeight;
		};

		TextureHandle m_TextureHandle;
		Dimensions m_Dimensions;
	};

	void InitStyle();

	void InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount);

	void DrawGUI(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);
	void DrawViewports();
	void DrawViewport(Viewport& viewport);

	void CreateViewport(Viewport& viewport);
	ViewportHandle AllocateViewportHandle();

	const ImGuiParams GetParams();

private:

	void BeginRender();

	void EndRender(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);

	ImVec2 CalculateViewportTextureSize(Viewport::Dimensions& const dims);

	ImTextureID TexHandleToImTexID(TextureHandle handle);

	ImGuiParams m_Params;

	std::unordered_map<ViewportHandle, Viewport> m_Viewports;
	ViewportHandle m_NextHandle;

	ImVec4* m_DefaultUIColours;
	ImGuiStyle m_DefaultUIStyle;
	ImFont* m_DefaultFont;
};

