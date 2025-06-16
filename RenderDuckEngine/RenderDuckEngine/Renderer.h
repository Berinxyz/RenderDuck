#pragma once
#include "EngineCore.h"

#include "d3dApp.h"
#include "d3dUtil.h"
#include "FrameResource.h"
#include "MathHelper.h"
#include "Camera.h"
#include "ShadowMap.h"
#include "Ssao.h"
#include "UIManager.h"

#include "RenderSettings.h"

#include "DescriptorHeapAllocator.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct RenderItem
{
    RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 m_World = MathHelper::Identity4x4();

    XMFLOAT4X4 m_TexTransform = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify obect data we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int m_NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT m_ObjCBIndex = -1;

    Material* m_Mat = nullptr;
    MeshGeometry* m_Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT m_IndexCount = 0;
    UINT m_StartIndexLocation = 0;
    int m_BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
    Opaque = 0,
    Debug,
    Sky,
    Count
};

class Renderer : public D3DApp, IRenderSettings
{
public:
    friend class UIManager;

    Renderer(HINSTANCE hInstance);
    Renderer(const Renderer& rhs) = delete;
    Renderer& operator=(const Renderer& rhs) = delete;
    ~Renderer();

    virtual bool Initialize()override;

    // render settings
    virtual void SetRenderToMainRTV(bool renderToMainRTV) override { m_RenderToRTV = renderToMainRTV; }
    virtual RenderSettings& GetRenderSettings() override { return m_RenderSettings; }

private:
    virtual void CreateRtvAndDsvDescriptorHeaps()override;
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMaterialBuffer(const GameTimer& gt);
    void UpdateShadowTransform(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateShadowPassCB(const GameTimer& gt);
    void UpdateSsaoCB(const GameTimer& gt);

    void LoadTextures();
    void BuildRootSignature();
    void BuildSsaoRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();
    void BuildSkullGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
    void DrawSceneToShadowMap();
    void DrawNormalsAndDepth();

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

    UINT CreateSrv(ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc);

    // Descriptor allocators - returns heap index
    UINT AllocateDescriptor();
    UINT AllocateDescriptors(u32 descriptorCount);

    static void ImGuiAllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandleStart);
    static void ImGuiFreeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    void BuildMainRTV();


    RenderSettings m_RenderSettings;

    std::shared_ptr<UIManager> m_UIManager;

    std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
    FrameResource* m_CurrFrameResource = nullptr;
    int m_CurrFrameResourceIndex = 0;

    u32 m_DescriptorCount;

    bool m_RenderToRTV;
    int m_MainRTVIndex;
    int m_MainSRVIndex;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_MainRTV;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_MainCpuRtv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_MainCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_MainGpuSrv;

    ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
    ComPtr<ID3D12RootSignature> m_SsaoRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> m_SrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> m_AllRitems;

    // Render items divided by PSO.
    std::vector<RenderItem*> m_RitemLayer[(int)RenderLayer::Count];

    UINT m_SkyTexHeapIndex = 0;
    UINT m_ShadowMapHeapIndex = 0;
    UINT m_SsaoAmbientMapIndex = 0;
    UINT m_SsaoHeapIndexStart = 0;

    UINT m_NullCubeSrvIndex = 0;
    UINT m_NullTexSrvIndex1 = 0;
    UINT m_NullTexSrvIndex2 = 0;

    CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullSrv;

    PassConstants m_MainPassCB;  // index 0 of pass cbuffer.
    PassConstants m_ShadowPassCB;// index 1 of pass cbuffer.

    Camera m_Camera;

    std::unique_ptr<ShadowMap> m_ShadowMap;

    std::unique_ptr<Ssao> m_Ssao;

    DirectX::BoundingSphere m_SceneBounds;

    float m_LightNearZ = 0.0f;
    float m_LightFarZ = 0.0f;
    XMFLOAT3 m_LightPosW;
    XMFLOAT4X4 m_LightView = MathHelper::Identity4x4();
    XMFLOAT4X4 m_LightProj = MathHelper::Identity4x4();
    XMFLOAT4X4 m_ShadowTransform = MathHelper::Identity4x4();

    float m_LightRotationAngle = 0.0f;
    XMFLOAT3 m_BaseLightDirections[3] = {
        XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(0.0f, -0.707f, -0.707f)
    };
    XMFLOAT3 m_RotatedLightDirections[3];

    POINT m_LastMousePos;
};