#pragma once
#include "EngineCore.h"
#include "EngineUtils.h"

#include "ECS/EntityForwardDeclarations.h"

#include "Settings.h"
#include "RenderSettings.h"
#include "CameraSettings.h"

#include "include/imgui/imgui.h"
#include "include/imgui/backends/imgui_impl_win32.h"
#include "include/imgui/backends/imgui_impl_dx12.h"

#include <functional>

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

class Renderer;
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
			, m_HierarchyWindow(false)
			, m_InspectorWindow(false)
		{
		}

		bool m_SettingsWindow;
		bool m_HierarchyWindow;
		bool m_InspectorWindow;
		bool m_ShowDemoWindow;
	};

	void InitStyle();

	void InitialiseForDX12(HWND window, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap, int swapchainBufferCount);
	void InitObjects(Renderer* renderer, EntityAdminRef entityAdmin, std::shared_ptr<RenderSettings> renderSettingsRef, std::shared_ptr<CameraSettings> cameraSettingsRef);

	void Render(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer);

	void SubmitViewportTexture(std::string textureName, GPUTextureHandle textureHandle, u32 textureWidth, u32 textureHeight);
	void CreateViewport();

	std::string GetDefaultViewName();
	bool DockspaceLayoutEnabled();

private:

	const float c_DragSpeed = 0.01;

	Renderer* m_Renderer;
	std::shared_ptr<EntityAdmin> m_EntityAdmin;
	EntityHandle m_SelectedEntity;

	std::shared_ptr<UISettings> m_UISettingsRef;
	std::shared_ptr<RenderSettings> m_RenderSettingsRef;
	std::shared_ptr<CameraSettings> m_CameraSettingsRef;

	// UI Draw functions
	void DrawImGui();
	void DrawViewports();
	void DestroyClosedViewports();
	void DrawMainMenuBar();
	void DrawSettingsWindow();
	void DrawSceneHierarchy();
	void DrawEntityInspector();

	void AddEntity();
	void DeleteEntity();
	
	// imgui utility
	struct MemberInfo
	{
		const char* m_Name;       
		ImGuiDataType m_DataType;
		u32 m_PropertyCount;
		void* m_Data;
	};

	void DrawPropertyData(const MemberInfo& memberInfo);
	void DrawColouredVec3(vec3& vec);
	void DrawColouredVec4(vec4& vec);
	void DrawProperty(IProperty* property);
	void DrawPropertyConfig(PropertyConfig& propertyConfig);

	// XML
	void SaveSettings();

	void CleanUp();

	// Utility Functions
	ViewportTextureHandle GetViewportTextureHandle(std::string debugName);
	ImVec2 CalculateViewportTextureSize(ImVec2& const textureDims, ImVec2& const windowDims);
	ImTextureID TexHandleToImTexID(D3D12_GPU_DESCRIPTOR_HANDLE handle);
	ViewportHandle AllocateViewportHandle();

	ActiveWindows m_ActiveWindows;
	std::unordered_map<std::string, std::function<void(IProperty*)>> m_ImguiPropertyFuncs;
	std::unordered_map<ViewportHandle, Viewport> m_Viewports;
	std::unordered_map<ViewportTextureHandle, ViewportTexture> m_ViewportDisplayTextureHandles;
	ViewportHandle m_NextViewportHandle;

	ImVec4* m_DefaultUIColours;
	ImFont* m_DefaultFont;
};

