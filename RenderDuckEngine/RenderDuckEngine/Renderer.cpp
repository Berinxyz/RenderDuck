//***************************************************************************************
// SsaoApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "Renderer.h"

#include "UploadBuffer.h"
#include "GeometryGenerator.h"

const int gNumFrameResources = 3;
const u32 c_MaxSrvDescriptors = 10000;


Renderer::Renderer(HINSTANCE hInstance)
    : D3DApp(hInstance)
    , m_DescriptorCount(0)
{
    // Estimate the scene bounding sphere manually since we know how the scene was constructed.
    // The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
    // the world space origin.  In general, you need to loop over every world space vertex
    // position and compute the bounding sphere.
    m_SceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_SceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

Renderer::~Renderer()
{
    if(m_d3dDevice != nullptr)
        FlushCommandQueue();
}

bool Renderer::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	m_Camera.SetPosition(0.0f, 2.0f, -15.0f);
 
    m_ShadowMap = std::make_unique<ShadowMap>(m_d3dDevice.Get(),
        2048, 2048);

    m_Ssao = std::make_unique<Ssao>(
        m_d3dDevice.Get(),
        m_CommandList.Get(),
        m_ClientWidth, m_ClientHeight);

	LoadTextures();
    BuildRootSignature();
    BuildSsaoRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildSkullGeometry();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    m_Ssao->SetPSOs(m_PSOs["ssao"].Get(), m_PSOs["ssaoBlur"].Get());

    m_UIManager = std::make_unique<UIManager>();
    m_UIManager->InitialiseForDX12(MainWnd(), m_d3dDevice.Get(), m_CommandQueue.Get(), m_SrvDescriptorHeap.Get(), s_SwapChainBufferCount);
    m_UIManager->InitStyle();
    m_UIManager->CreateViewport();

    // Execute the initialization commands.
    ThrowIfFailed(m_CommandList->Close());
    ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}

UINT Renderer::AllocateDescriptors(u32 descriptorCount)
{
    UINT startIndex = -1;
    for (int i = 0; i < descriptorCount; ++i)
    {
        if (i >  0)
        {
            AllocateDescriptor();
        }
        else
        {
            startIndex = AllocateDescriptor();
        }
    }

    return startIndex;
}

void Renderer::ImGuiAllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
{
    s_SrvDescriptorHeapAllocator.Alloc(outCpuHandle, outGpuHandle);
}

void Renderer::ImGuiFreeDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    s_SrvDescriptorHeapAllocator.Free(cpuHandle, gpuHandle);

}

UINT Renderer::AllocateDescriptor()
{
    return s_SrvDescriptorHeapAllocator.Alloc();
}

void Renderer::CreateRtvAndDsvDescriptorHeaps()
{
    // Add +1 for screen normal map, +2 for ambient maps.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = s_SwapChainBufferCount + 4;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())));

    // Add +1 DSV for shadow map.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 2;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())));
}
 
void Renderer::OnResize()
{
    D3DApp::OnResize();

	m_Camera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

    if(m_Ssao != nullptr)
    {
        m_Ssao->OnResize(m_ClientWidth, m_ClientHeight);

        // Resources changed, so need to rebuild descriptors.
        m_Ssao->RebuildDescriptors(m_DepthStencilBuffer.Get());
    }
}

void Renderer::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);

    // Cycle through the circular frame resource array.
    m_CurrFrameResourceIndex = (m_CurrFrameResourceIndex + 1) % gNumFrameResources;
    m_CurrFrameResource = m_FrameResources[m_CurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(m_CurrFrameResource->Fence != 0 && m_Fence->GetCompletedValue() < m_CurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_Fence->SetEventOnCompletion(m_CurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    //
    // Animate the lights (and hence shadows).
    //

    m_LightRotationAngle += 0.1f*gt.DeltaTime();

    XMMATRIX R = XMMatrixRotationY(m_LightRotationAngle);
    for(int i = 0; i < 3; ++i)
    {
        XMVECTOR lightDir = XMLoadFloat3(&m_BaseLightDirections[i]);
        lightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&m_RotatedLightDirections[i], lightDir);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
    UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
    UpdateShadowPassCB(gt);
    UpdateSsaoCB(gt);
}

void Renderer::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
    m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	//
	// Shadow map pass.
	//

    // Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
    // set as a root descriptor.
    auto matBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
    m_CommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
	
    // Bind null SRV for shadow map pass.
    m_CommandList->SetGraphicsRootDescriptorTable(3, m_NullSrv);	 

    // Bind all the textures used in this scene.  Observe
    // that we only have to specify the first descriptor in the table.  
    // The root signature knows how many descriptors are expected in the table.
    m_CommandList->SetGraphicsRootDescriptorTable(4, m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    DrawSceneToShadowMap();

	//
	// Normal/depth pass.
	//
	
	DrawNormalsAndDepth();
	
	//
	// Compute SSAO.
	// 
	
    m_CommandList->SetGraphicsRootSignature(m_SsaoRootSignature.Get());
    m_Ssao->ComputeSsao(m_CommandList.Get(), m_CurrFrameResource, 3);
	
	//
	// Main rendering pass.
	//
	
    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

    // Rebind state whenever graphics root signature changes.

    // Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
    // set as a root descriptor.
    matBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
    m_CommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());


    m_CommandList->RSSetViewports(1, &m_ScreenViewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

    // Specify the buffers we are going to render to.
    if (!m_UIManager->GetParams().m_DockSpace)
    {
        m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
        m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
        m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    }
    else
    {
        m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_MainRTV.Get(),
            D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
        m_CommandList->ClearRenderTargetView(m_MainCpuRtv, Colors::LightSteelBlue, 0, nullptr);        
        m_CommandList->OMSetRenderTargets(1, &m_MainCpuRtv, true, &DepthStencilView());
        
    }

	// Bind all the textures used in this scene.  Observe
    // that we only have to specify the first descriptor in the table.  
    // The root signature knows how many descriptors are expected in the table.
    m_CommandList->SetGraphicsRootDescriptorTable(4, m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
    auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    // Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
    // from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
    // If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
    // index into an array of cube maps.

    CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    skyTexDescriptor.Offset(m_SkyTexHeapIndex, m_CbvSrvUavDescriptorSize);
    m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);


    m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
    DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Sky]);

    m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
    DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);

    //m_CommandList->SetPipelineState(m_PSOs["debug"].Get());
    //DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Debug]);

    // Indicate a state transition on the resource usage.
    if (m_UIManager->GetParams().m_DockSpace)
    {
        m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_MainRTV.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
        m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
        m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    }
    
    // submit debug views
    m_UIManager->SubmitViewportTexture("Scene Normals", m_Ssao->NormalMapSrv(), m_ClientWidth, m_ClientHeight);
    m_UIManager->SubmitViewportTexture("SSAO", m_Ssao->AmbientMapSrv(), m_ClientWidth, m_ClientHeight);
    m_UIManager->SubmitViewportTexture(m_UIManager->GetDefaultViewName(), m_MainGpuSrv, m_ClientWidth, m_ClientHeight);

    // Draw Imui
    m_UIManager->Render(m_CommandList.Get(), CurrentBackBuffer());

    
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(m_CommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(m_SwapChain->Present(0, 0));
	m_CurrBackBuffer = (m_CurrBackBuffer + 1) % s_SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    m_CurrFrameResource->Fence = ++m_CurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
}

void Renderer::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_LastMousePos.x = x;
    m_LastMousePos.y = y;

    SetCapture(m_hMainWnd);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(0, true);

}

void Renderer::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(0, false);
}

void Renderer::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_RBUTTON) != 0)
    {
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_LastMousePos.y));

		m_Camera.Pitch(dy);
		m_Camera.RotateY(dx);
    }

    m_LastMousePos.x = x;
    m_LastMousePos.y = y;
}
 
void Renderer::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if(GetAsyncKeyState('W') & 0x8000)
		m_Camera.Walk(10.0f*dt);

	if(GetAsyncKeyState('S') & 0x8000)
		m_Camera.Walk(-10.0f*dt);

	if(GetAsyncKeyState('A') & 0x8000)
		m_Camera.Strafe(-10.0f*dt);

	if(GetAsyncKeyState('D') & 0x8000)
		m_Camera.Strafe(10.0f*dt);

	m_Camera.UpdateViewMatrix();
}
 
void Renderer::AnimateMaterials(const GameTimer& gt)
{
	
}

void Renderer::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for(auto& e : m_AllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->m_NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->m_World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->m_TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e->m_Mat->MatCBIndex;

			currObjectCB->CopyData(e->m_ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->m_NumFramesDirty--;
		}
	}
}

void Renderer::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialBuffer = m_CurrFrameResource->MaterialBuffer.get();
	for(auto& e : m_Materials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			matData.NormalMapIndex = mat->NormalSrvHeapIndex;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void Renderer::UpdateShadowTransform(const GameTimer& gt)
{
    // Only the first "main" light casts a shadow.
    XMVECTOR lightDir = XMLoadFloat3(&m_RotatedLightDirections[0]);
    XMVECTOR lightPos = -2.0f*m_SceneBounds.Radius*lightDir;
    XMVECTOR targetPos = XMLoadFloat3(&m_SceneBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMStoreFloat3(&m_LightPosW, lightPos);

    // Transform bounding sphere to light space.
    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    // Ortho frustum in light space encloses scene.
    float l = sphereCenterLS.x - m_SceneBounds.Radius;
    float b = sphereCenterLS.y - m_SceneBounds.Radius;
    float n = sphereCenterLS.z - m_SceneBounds.Radius;
    float r = sphereCenterLS.x + m_SceneBounds.Radius;
    float t = sphereCenterLS.y + m_SceneBounds.Radius;
    float f = sphereCenterLS.z + m_SceneBounds.Radius;

    m_LightNearZ = n;
    m_LightFarZ = f;
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX S = lightView*lightProj*T;
    XMStoreFloat4x4(&m_LightView, lightView);
    XMStoreFloat4x4(&m_LightProj, lightProj);
    XMStoreFloat4x4(&m_ShadowTransform, S);
}

void Renderer::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = m_Camera.GetView();
	XMMATRIX proj = m_Camera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
    XMMATRIX shadowTransform = XMLoadFloat4x4(&m_ShadowTransform);

	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    XMStoreFloat4x4(&m_MainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
    XMStoreFloat4x4(&m_MainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	m_MainPassCB.EyePosW = m_Camera.GetPosition3f();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.TotalTime();
	m_MainPassCB.DeltaTime = gt.DeltaTime();
	m_MainPassCB.AmbientLight = { 0.4f, 0.4f, 0.6f, 1.0f };
	m_MainPassCB.Lights[0].Direction = m_RotatedLightDirections[0];
	m_MainPassCB.Lights[0].Strength = { 0.4f, 0.4f, 0.5f };
	m_MainPassCB.Lights[1].Direction = m_RotatedLightDirections[1];
	m_MainPassCB.Lights[1].Strength = { 0.1f, 0.1f, 0.1f };
	m_MainPassCB.Lights[2].Direction = m_RotatedLightDirections[2];
	m_MainPassCB.Lights[2].Strength = { 0.0f, 0.0f, 0.0f };
 
	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void Renderer::UpdateShadowPassCB(const GameTimer& gt)
{
    XMMATRIX view = XMLoadFloat4x4(&m_LightView);
    XMMATRIX proj = XMLoadFloat4x4(&m_LightProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    UINT w = m_ShadowMap->Width();
    UINT h = m_ShadowMap->Height();

    XMStoreFloat4x4(&m_ShadowPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&m_ShadowPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&m_ShadowPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&m_ShadowPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&m_ShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&m_ShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    m_ShadowPassCB.EyePosW = m_LightPosW;
    m_ShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
    m_ShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
    m_ShadowPassCB.NearZ = m_LightNearZ;
    m_ShadowPassCB.FarZ = m_LightFarZ;

    auto currPassCB = m_CurrFrameResource->PassCB.get();
    currPassCB->CopyData(1, m_ShadowPassCB);
}

void Renderer::UpdateSsaoCB(const GameTimer& gt)
{
    SsaoConstants ssaoCB;

    XMMATRIX P = m_Camera.GetProj();

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    ssaoCB.Proj    = m_MainPassCB.Proj;
    ssaoCB.InvProj = m_MainPassCB.InvProj;
    XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P*T));

    m_Ssao->GetOffsetVectors(ssaoCB.OffsetVectors);

    auto blurWeights = m_Ssao->CalcGaussWeights(2.5f);
    ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
    ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
    ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

    ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_Ssao->SsaoMapWidth(), 1.0f / m_Ssao->SsaoMapHeight());

    // Coordinates given in view space.
    ssaoCB.OcclusionRadius = 0.5f;
    ssaoCB.OcclusionFadeStart = 0.2f;
    ssaoCB.OcclusionFadeEnd = 1.0f;
    ssaoCB.SurfaceEpsilon = 0.05f;
 
    auto currSsaoCB = m_CurrFrameResource->SsaoCB.get();
    currSsaoCB->CopyData(0, ssaoCB);
}

void Renderer::LoadTextures()
{
	std::vector<std::string> texNames = 
	{
		"bricksDiffuseMap",
		"bricksNormalMap",
		"tileDiffuseMap",
		"tileNormalMap",
		"defaultDiffuseMap",
		"defaultNormalMap",
		"skyCubeMap"
	};
	
    std::vector<std::wstring> texFilenames =
    {
        L"Assets/Textures/bricks2.dds",
        L"Assets/Textures/bricks2_nmap.dds",
        L"Assets/Textures/tile.dds",
        L"Assets/Textures/tile_nmap.dds",
        L"Assets/Textures/white1x1.dds",
        L"Assets/Textures/default_nmap.dds",
        L"Assets/Textures/sunsetcube1024.dds"
    };
	
	for(int i = 0; i < (int)texNames.size(); ++i)
	{
		auto texMap = std::make_unique<Texture>();
		texMap->Name = texNames[i];
		texMap->Filename = texFilenames[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_d3dDevice.Get(),
			m_CommandList.Get(), texMap->Filename.c_str(),
			texMap->Resource, texMap->UploadHeap));
			
		m_Textures[texMap->Name] = std::move(texMap);
	}		
}

void Renderer::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 3, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(m_d3dDevice->CreateRootSignature(
		0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void Renderer::BuildSsaoRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE texTable0;
    texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

    CD3DX12_DESCRIPTOR_RANGE texTable1;
    texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstants(1, 1);
    slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
        0.0f,
        0,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE); 

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
    {
        pointClamp, linearClamp, depthMapSam, linearWrap
    };

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(m_d3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(m_SsaoRootSignature.GetAddressOf())));
}

void Renderer::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = c_MaxSrvDescriptors;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

    s_SrvDescriptorHeapAllocator.Create(m_d3dDevice.Get(), m_SrvDescriptorHeap.Get());

	//
	// Fill out the heap with actual descriptors.
	//

	std::vector<ComPtr<ID3D12Resource>> tex2DList = 
	{
		m_Textures["bricksDiffuseMap"]->Resource,
		m_Textures["bricksNormalMap"]->Resource,
		m_Textures["tileDiffuseMap"]->Resource,
		m_Textures["tileNormalMap"]->Resource,
		m_Textures["defaultDiffuseMap"]->Resource,
		m_Textures["defaultNormalMap"]->Resource
	};
	
	auto skyCubeMap = m_Textures["skyCubeMap"]->Resource;

    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        for (UINT i = 0; i < (UINT)tex2DList.size(); ++i)
        {
            srvDesc.Format = tex2DList[i]->GetDesc().Format;
            srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
            CreateSrv(tex2DList[i].Get(), &srvDesc);
        }

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
        srvDesc.Format = skyCubeMap->GetDesc().Format;
        m_SkyTexHeapIndex = CreateSrv(skyCubeMap.Get(), &srvDesc);

        m_ShadowMapHeapIndex = AllocateDescriptor();
        m_SsaoHeapIndexStart = AllocateDescriptor();
        m_SsaoAmbientMapIndex = AllocateDescriptors(3);
        m_NullCubeSrvIndex = AllocateDescriptors(5);
        m_NullTexSrvIndex1 = AllocateDescriptor();
        m_NullTexSrvIndex2 = AllocateDescriptor();
        m_MainRTVIndex = AllocateDescriptor();
        m_MainSRVIndex = AllocateDescriptor();

        auto nullSrv = GetCpuSrv(m_NullCubeSrvIndex);
        m_NullSrv = GetGpuSrv(m_NullCubeSrvIndex);

        m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
        nullSrv.Offset(1, m_CbvSrvUavDescriptorSize);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

        nullSrv.Offset(1, m_CbvSrvUavDescriptorSize);
        m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

        m_ShadowMap->BuildDescriptors(
            GetCpuSrv(m_ShadowMapHeapIndex),
            GetGpuSrv(m_ShadowMapHeapIndex),
            GetDsv(1));

        m_Ssao->BuildDescriptors(
            m_DepthStencilBuffer.Get(),
            GetCpuSrv(m_SsaoHeapIndexStart),
            GetGpuSrv(m_SsaoHeapIndexStart),
            GetRtv(s_SwapChainBufferCount),
            m_CbvSrvUavDescriptorSize,
            m_RtvDescriptorSize);
    }

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = m_ClientWidth;
    texDesc.Height = m_ClientHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(rtvDesc.Format, ambientClearColor);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&m_MainRTV)));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    m_MainCpuSrv = GetCpuSrv(m_MainRTVIndex);
    m_MainGpuSrv = GetGpuSrv(m_MainRTVIndex);
    m_MainCpuRtv = GetRtv(s_SwapChainBufferCount + 3);

    m_d3dDevice->CreateRenderTargetView(m_MainRTV.Get(), &rtvDesc, m_MainCpuRtv);
    m_d3dDevice->CreateShaderResourceView(m_MainRTV.Get(), &srvDesc, m_MainCpuSrv);
}

void Renderer::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

    m_Shaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
    m_Shaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
    m_Shaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
    m_Shaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
    m_Shaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

    m_Shaders["drawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
    m_Shaders["drawNormalsPS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

    m_Shaders["ssaoVS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
    m_Shaders["ssaoPS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

    m_Shaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
    m_Shaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

    m_InputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

void Renderer::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
    GeometryGenerator::MeshData quad = geoGen.CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    
	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
    UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
    UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    SubmeshGeometry quadSubmesh;
    quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
    quadSubmesh.StartIndexLocation = quadIndexOffset;
    quadSubmesh.BaseVertexLocation = quadVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() + 
        quad.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for(size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}

	for(size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
		vertices[k].TangentU = grid.Vertices[i].TangentU;
	}

	for(size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	for(size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
	}

    for(int i = 0; i < quad.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = quad.Vertices[i].Position;
        vertices[k].Normal = quad.Vertices[i].Normal;
        vertices[k].TexC = quad.Vertices[i].TexC;
        vertices[k].TangentU = quad.Vertices[i].TangentU;
    }

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
    indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size()  * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_d3dDevice.Get(),
		m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_d3dDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
    geo->DrawArgs["quad"] = quadSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void Renderer::BuildSkullGeometry()
{
    std::ifstream fin("Assets/Models/skull.txt");

    if (!fin)
    {
        MessageBox(0, L"Assets/Models/skull.txt not found.", 0, 0);
        return;
    }

    UINT vcount = 0;
    UINT tcount = 0;
    std::string ignore;

    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
    XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

    XMVECTOR vMin = XMLoadFloat3(&vMinf3);
    XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

    std::vector<Vertex> vertices(vcount);
    for (UINT i = 0; i < vcount; ++i)
    {
        fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
        fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

        vertices[i].TexC = { 0.0f, 0.0f };

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        XMVECTOR N = XMLoadFloat3(&vertices[i].Normal);

        // Generate a tangent vector so normal mapping works.  We aren't applying
        // a texture map to the skull, so we just need any tangent vector so that
        // the math works out to give us the original interpolated vertex normal.
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        if (fabsf(XMVectorGetX(XMVector3Dot(N, up))) < 1.0f - 0.001f)
        {
            XMVECTOR T = XMVector3Normalize(XMVector3Cross(up, N));
            XMStoreFloat3(&vertices[i].TangentU, T);
        }
        else
        {
            up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            XMVECTOR T = XMVector3Normalize(XMVector3Cross(N, up));
            XMStoreFloat3(&vertices[i].TangentU, T);
        }


        vMin = XMVectorMin(vMin, P);
        vMax = XMVectorMax(vMax, P);
    }

    BoundingBox bounds;
    XMStoreFloat3(&bounds.Center, 0.5f*(vMin + vMax));
    XMStoreFloat3(&bounds.Extents, 0.5f*(vMax - vMin));

    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> indices(3 * tcount);
    for (UINT i = 0; i < tcount; ++i)
    {
        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
    }

    fin.close();

    //
    // Pack the indices of all the meshes into one index buffer.
    //

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "skullGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_d3dDevice.Get(),
        m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_d3dDevice.Get(),
        m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    submesh.Bounds = bounds;

    geo->DrawArgs["skull"] = submesh;

    m_Geometries[geo->Name] = std::move(geo);
}

void Renderer::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;

	
    ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    basePsoDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
    basePsoDesc.pRootSignature = m_RootSignature.Get();
    basePsoDesc.VS =
	{ 
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()), 
		m_Shaders["standardVS"]->GetBufferSize()
	};
    basePsoDesc.PS =
	{ 
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
    basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    basePsoDesc.SampleMask = UINT_MAX;
    basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    basePsoDesc.NumRenderTargets = 1;
    basePsoDesc.RTVFormats[0] = m_BackBufferFormat;
    basePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
    basePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
    basePsoDesc.DSVFormat = m_DepthStencilFormat;

    //
    // PSO for opaque objects.
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = basePsoDesc;
    opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

    //
    // PSO for shadow map pass.
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = basePsoDesc;
    smapPsoDesc.RasterizerState.DepthBias = 100000;
    smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
    smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
    smapPsoDesc.pRootSignature = m_RootSignature.Get();
    smapPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["shadowVS"]->GetBufferPointer()),
        m_Shaders["shadowVS"]->GetBufferSize()
    };
    smapPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["shadowOpaquePS"]->GetBufferPointer()),
        m_Shaders["shadowOpaquePS"]->GetBufferSize()
    };
    
    // Shadow map pass does not have a render target.
    smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    smapPsoDesc.NumRenderTargets = 0;
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&m_PSOs["shadow_opaque"])));

    //
    // PSO for debug layer.
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPsoDesc = basePsoDesc;
    debugPsoDesc.pRootSignature = m_RootSignature.Get();
    debugPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["debugVS"]->GetBufferPointer()),
        m_Shaders["debugVS"]->GetBufferSize()
    };
    debugPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["debugPS"]->GetBufferPointer()),
        m_Shaders["debugPS"]->GetBufferSize()
    };
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&debugPsoDesc, IID_PPV_ARGS(&m_PSOs["debug"])));

    //
    // PSO for drawing normals.
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = basePsoDesc;
    drawNormalsPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["drawNormalsVS"]->GetBufferPointer()),
        m_Shaders["drawNormalsVS"]->GetBufferSize()
    };
    drawNormalsPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["drawNormalsPS"]->GetBufferPointer()),
        m_Shaders["drawNormalsPS"]->GetBufferSize()
    };
    drawNormalsPsoDesc.RTVFormats[0] = Ssao::NormalMapFormat;
    drawNormalsPsoDesc.SampleDesc.Count = 1;
    drawNormalsPsoDesc.SampleDesc.Quality = 0;
    drawNormalsPsoDesc.DSVFormat = m_DepthStencilFormat;
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&m_PSOs["drawNormals"])));

    //
    // PSO for SSAO.
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = basePsoDesc;
    ssaoPsoDesc.InputLayout = { nullptr, 0 };
    ssaoPsoDesc.pRootSignature = m_SsaoRootSignature.Get();
    ssaoPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["ssaoVS"]->GetBufferPointer()),
        m_Shaders["ssaoVS"]->GetBufferSize()
    };
    ssaoPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["ssaoPS"]->GetBufferPointer()),
        m_Shaders["ssaoPS"]->GetBufferSize()
    };

    // SSAO effect does not need the depth buffer.
    ssaoPsoDesc.DepthStencilState.DepthEnable = false;
    ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
    ssaoPsoDesc.SampleDesc.Count = 1;
    ssaoPsoDesc.SampleDesc.Quality = 0;
    ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&m_PSOs["ssao"])));

    //
    // PSO for SSAO blur.
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
    ssaoBlurPsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["ssaoBlurVS"]->GetBufferPointer()),
        m_Shaders["ssaoBlurVS"]->GetBufferSize()
    };
    ssaoBlurPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(m_Shaders["ssaoBlurPS"]->GetBufferPointer()),
        m_Shaders["ssaoBlurPS"]->GetBufferSize()
    };
    ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&m_PSOs["ssaoBlur"])));

	//
	// PSO for sky.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = basePsoDesc;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = m_RootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyVS"]->GetBufferPointer()),
		m_Shaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyPS"]->GetBufferPointer()),
		m_Shaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&m_PSOs["sky"])));

}

void Renderer::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        m_FrameResources.push_back(std::make_unique<FrameResource>(m_d3dDevice.Get(),
            2, (UINT)m_AllRitems.size(), (UINT)m_Materials.size()));
    }
}

void Renderer::BuildMaterials()
{
    auto bricks0 = std::make_unique<Material>();
    bricks0->Name = "bricks0";
    bricks0->MatCBIndex = 0;
    bricks0->DiffuseSrvHeapIndex = 0;
    bricks0->NormalSrvHeapIndex = 1;
    bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    bricks0->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    bricks0->Roughness = 0.3f;

    auto tile0 = std::make_unique<Material>();
    tile0->Name = "tile0";
    tile0->MatCBIndex = 2;
    tile0->DiffuseSrvHeapIndex = 2;
    tile0->NormalSrvHeapIndex = 3;
    tile0->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
    tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
    tile0->Roughness = 0.1f;

    auto mirror0 = std::make_unique<Material>();
    mirror0->Name = "mirror0";
    mirror0->MatCBIndex = 3;
    mirror0->DiffuseSrvHeapIndex = 4;
    mirror0->NormalSrvHeapIndex = 5;
    mirror0->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    mirror0->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
    mirror0->Roughness = 0.1f;

    auto skullMat = std::make_unique<Material>();
    skullMat->Name = "skullMat";
    skullMat->MatCBIndex = 3;
    skullMat->DiffuseSrvHeapIndex = 4;
    skullMat->NormalSrvHeapIndex = 5;
    skullMat->DiffuseAlbedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
    skullMat->FresnelR0 = XMFLOAT3(0.6f, 0.6f, 0.6f);
    skullMat->Roughness = 0.2f;

    auto sky = std::make_unique<Material>();
    sky->Name = "sky";
    sky->MatCBIndex = 4;
    sky->DiffuseSrvHeapIndex = 6;
    sky->NormalSrvHeapIndex = 7;
    sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    sky->Roughness = 1.0f;

    m_Materials["bricks0"] = std::move(bricks0);
    m_Materials["tile0"] = std::move(tile0);
    m_Materials["mirror0"] = std::move(mirror0);
    m_Materials["skullMat"] = std::move(skullMat);
    m_Materials["sky"] = std::move(sky);
}

void Renderer::BuildRenderItems()
{
	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->m_World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->m_TexTransform = MathHelper::Identity4x4();
	skyRitem->m_ObjCBIndex = 0;
	skyRitem->m_Mat = m_Materials["sky"].get();
	skyRitem->m_Geo = m_Geometries["shapeGeo"].get();
	skyRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->m_IndexCount = skyRitem->m_Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->m_StartIndexLocation = skyRitem->m_Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->m_BaseVertexLocation = skyRitem->m_Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	m_AllRitems.push_back(std::move(skyRitem));
    
    auto quadRitem = std::make_unique<RenderItem>();
    quadRitem->m_World = MathHelper::Identity4x4();
    quadRitem->m_TexTransform = MathHelper::Identity4x4();
    quadRitem->m_ObjCBIndex = 1;
    quadRitem->m_Mat = m_Materials["bricks0"].get();
    quadRitem->m_Geo = m_Geometries["shapeGeo"].get();
    quadRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    quadRitem->m_IndexCount = quadRitem->m_Geo->DrawArgs["quad"].IndexCount;
    quadRitem->m_StartIndexLocation = quadRitem->m_Geo->DrawArgs["quad"].StartIndexLocation;
    quadRitem->m_BaseVertexLocation = quadRitem->m_Geo->DrawArgs["quad"].BaseVertexLocation;

    m_RitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
    m_AllRitems.push_back(std::move(quadRitem));
    
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->m_World, XMMatrixScaling(2.0f, 1.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	XMStoreFloat4x4(&boxRitem->m_TexTransform, XMMatrixScaling(1.0f, 0.5f, 1.0f));
	boxRitem->m_ObjCBIndex = 2;
	boxRitem->m_Mat = m_Materials["bricks0"].get();
	boxRitem->m_Geo = m_Geometries["shapeGeo"].get();
	boxRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->m_IndexCount = boxRitem->m_Geo->DrawArgs["box"].IndexCount;
	boxRitem->m_StartIndexLocation = boxRitem->m_Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->m_BaseVertexLocation = boxRitem->m_Geo->DrawArgs["box"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	m_AllRitems.push_back(std::move(boxRitem));

    auto skullRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&skullRitem->m_World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    skullRitem->m_TexTransform = MathHelper::Identity4x4();
    skullRitem->m_ObjCBIndex = 3;
    skullRitem->m_Mat = m_Materials["skullMat"].get();
    skullRitem->m_Geo = m_Geometries["skullGeo"].get();
    skullRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullRitem->m_IndexCount = skullRitem->m_Geo->DrawArgs["skull"].IndexCount;
    skullRitem->m_StartIndexLocation = skullRitem->m_Geo->DrawArgs["skull"].StartIndexLocation;
    skullRitem->m_BaseVertexLocation = skullRitem->m_Geo->DrawArgs["skull"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());
	m_AllRitems.push_back(std::move(skullRitem));

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->m_World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->m_TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->m_ObjCBIndex = 4;
	gridRitem->m_Mat = m_Materials["tile0"].get();
	gridRitem->m_Geo = m_Geometries["shapeGeo"].get();
	gridRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->m_IndexCount = gridRitem->m_Geo->DrawArgs["grid"].IndexCount;
    gridRitem->m_StartIndexLocation = gridRitem->m_Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->m_BaseVertexLocation = gridRitem->m_Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	m_AllRitems.push_back(std::move(gridRitem));

	XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
	UINT objCBIndex = 5;
	for(int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);

		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);

		XMStoreFloat4x4(&leftCylRitem->m_World, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->m_TexTransform, brickTexTransform);
		leftCylRitem->m_ObjCBIndex = objCBIndex++;
		leftCylRitem->m_Mat = m_Materials["bricks0"].get();
		leftCylRitem->m_Geo = m_Geometries["shapeGeo"].get();
		leftCylRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->m_IndexCount = leftCylRitem->m_Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->m_StartIndexLocation = leftCylRitem->m_Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->m_BaseVertexLocation = leftCylRitem->m_Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->m_World, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->m_TexTransform, brickTexTransform);
		rightCylRitem->m_ObjCBIndex = objCBIndex++;
		rightCylRitem->m_Mat = m_Materials["bricks0"].get();
		rightCylRitem->m_Geo = m_Geometries["shapeGeo"].get();
		rightCylRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->m_IndexCount = rightCylRitem->m_Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->m_StartIndexLocation = rightCylRitem->m_Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->m_BaseVertexLocation = rightCylRitem->m_Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->m_World, leftSphereWorld);
		leftSphereRitem->m_TexTransform = MathHelper::Identity4x4();
		leftSphereRitem->m_ObjCBIndex = objCBIndex++;
		leftSphereRitem->m_Mat = m_Materials["mirror0"].get();
		leftSphereRitem->m_Geo = m_Geometries["shapeGeo"].get();
		leftSphereRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->m_IndexCount = leftSphereRitem->m_Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->m_StartIndexLocation = leftSphereRitem->m_Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->m_BaseVertexLocation = leftSphereRitem->m_Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->m_World, rightSphereWorld);
		rightSphereRitem->m_TexTransform = MathHelper::Identity4x4();
		rightSphereRitem->m_ObjCBIndex = objCBIndex++;
		rightSphereRitem->m_Mat = m_Materials["mirror0"].get();
		rightSphereRitem->m_Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->m_PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->m_IndexCount = rightSphereRitem->m_Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->m_StartIndexLocation = rightSphereRitem->m_Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->m_BaseVertexLocation = rightSphereRitem->m_Geo->DrawArgs["sphere"].BaseVertexLocation;

		m_RitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		m_RitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		m_RitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
		m_RitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

		m_AllRitems.push_back(std::move(leftCylRitem));
		m_AllRitems.push_back(std::move(rightCylRitem));
		m_AllRitems.push_back(std::move(leftSphereRitem));
		m_AllRitems.push_back(std::move(rightSphereRitem));
	}
}

void Renderer::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
 
	auto objectCB = m_CurrFrameResource->ObjectCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->m_Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->m_Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->m_PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->m_ObjCBIndex*objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        cmdList->DrawIndexedInstanced(ri->m_IndexCount, 1, ri->m_StartIndexLocation, ri->m_BaseVertexLocation, 0);
    }
}

void Renderer::DrawSceneToShadowMap()
{
    m_CommandList->RSSetViewports(1, &m_ShadowMap->Viewport());
    m_CommandList->RSSetScissorRects(1, &m_ShadowMap->ScissorRect());

    // Change to DEPTH_WRITE.
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->Resource(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Clear the back buffer and depth buffer.
    m_CommandList->ClearDepthStencilView(m_ShadowMap->Dsv(), 
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    m_CommandList->OMSetRenderTargets(0, nullptr, false, &m_ShadowMap->Dsv());

    // Bind the pass constant buffer for the shadow map pass.
    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    auto passCB = m_CurrFrameResource->PassCB->Resource();
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1*passCBByteSize;
    m_CommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

    m_CommandList->SetPipelineState(m_PSOs["shadow_opaque"].Get());

    DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->Resource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}
 
void Renderer::DrawNormalsAndDepth()
{
	m_CommandList->RSSetViewports(1, &m_ScreenViewport);
    m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	auto normalMap = m_Ssao->NormalMap();
	auto normalMapRtv = m_Ssao->NormalMapRtv();
	
    // Change to RENDER_TARGET.
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float clearValue[] = {0.0f, 0.0f, 1.0f, 0.0f};
    m_CommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
    m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
    m_CommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

    // Bind the constant buffer for this pass.
    auto passCB = m_CurrFrameResource->PassCB->Resource();
    m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

    m_CommandList->SetPipelineState(m_PSOs["drawNormals"].Get());

    DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::GetCpuSrv(int index)const
{
    auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    srv.Offset(index, m_CbvSrvUavDescriptorSize);
    return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Renderer::GetGpuSrv(int index)const
{
    auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    srv.Offset(index, m_CbvSrvUavDescriptorSize);
    return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::GetDsv(int index)const
{
    auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DsvHeap->GetCPUDescriptorHandleForHeapStart());
    dsv.Offset(index, m_DsvDescriptorSize);
    return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::GetRtv(int index)const
{
    auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtv.Offset(index, m_RtvDescriptorSize);
    return rtv;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> Renderer::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC shadow(
        6, // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
        0.0f,                               // mipLODBias
        16,                                 // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp,
        shadow 
    };
}

UINT Renderer::CreateSrv(ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{    
    UINT idx = AllocateDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE& descriptorHandle = GetCpuSrv(idx);
    m_d3dDevice->CreateShaderResourceView(resource.Get(), desc, descriptorHandle);
    return idx;
}

