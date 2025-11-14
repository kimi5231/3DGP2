#include "pch.h"
#include "GameFramework.h"
#include "Scene.h"

GameFramework::GameFramework(HWND hwnd)
{
	_hwnd = hwnd;

	CreateDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvHeaps();
	CreateSwapChine();
	CreateDepthStencilView();

	_commandList->Reset(_commandAllocator.Get(), NULL);

	// 씬 객체 생성
	now_scene = new Scene(_device, _commandList);
	
	// 그래픽 명령 리스트 명령 큐에 추가
	_commandList->Close();
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, commandLists);

	//그래픽 명령 리스트들이 모두 실행될 때까지 기다린다.
	WaitForGpuComplete();

	//그래픽 리소스들을 생성하는 과정에 생성된 업로드 버퍼들을 소멸시킨다.
	if (now_scene) now_scene->ReleaseUploadBuffers();

}

GameFramework::~GameFramework()
{
}

void GameFramework::Update()
{
	// 명령 리셋
	_commandAllocator->Reset();
	_commandList->Reset(_commandAllocator.Get(), NULL);

	// 뷰포트 씨저 사각형 설정
	_commandList->RSSetViewports(1, &viewport);
	_commandList->RSSetScissorRects(1, &scissor_rect);

	// 현재 렌더 타겟에 대한 프리젠트가 끝나기를 기다림. 프리젠트가 끝나면 렌더 타겟 상태로 바꿈
	D3D12_RESOURCE_BARRIER resourceBarrier{};
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// render target state로 리소스 변경
	resourceBarrier.Transition.pResource = render_target_buffers[swap_chain_buffer_index].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &resourceBarrier);

	// 현재 렌더 타겟에 해당하는 서술자의 CPU 주소(핸들) 값을 계산
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDesciptorHandle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	rtvCPUDesciptorHandle.ptr += (swap_chain_buffer_index * rtv_increment_size);

	// 깊이-스텐실 서술자의 CPU 주소를 계산한다.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

	// 렌더 타겟 뷰와 깊이 스텐실 뷰를 출력 병합 단계(OM)에 연결
	_commandList->OMSetRenderTargets(1, &rtvCPUDesciptorHandle, FALSE, &dsvCPUDescriptorHandle);

	// 원하는 색상으로 렌더 타겟 지우기
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	_commandList->ClearRenderTargetView(rtvCPUDesciptorHandle, clearColor, 0, NULL);

	// 원하는 값으로 깊이 스텐실 지우기
	_commandList->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, NULL);

	// 렌더링 코드
	if (now_scene) now_scene->Render(_commandList.Get());

	// 현재 렌더 타겟에 대한 렌더링이 끝나기를 기다림. GPU가 버퍼를 더 이상 사용하지 않으면 렌더 타겟 -> 프레젠트 상태로 변경
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &resourceBarrier);

	_commandList->Close();

	ID3D12CommandList* commandLists[]{ _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, commandLists);

	WaitForGpuComplete();

	// 스왑체인 프리젠트. 현재 렌더 타겟의 내용이 전면 버퍼로 옮겨지고 렌더 타겟 인덱스가 바뀜
	_swapChain->Present(0, 0);

	MoveToNextFrame();
}

void GameFramework::Render()
{
	
}

void GameFramework::CreateDevice()
{
	HRESULT result;
	UINT dxgiFactoryFlags{};
#if defined(_DEBUG)
	ID3D12Debug* d3dDebugController{};
	result = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&d3dDebugController);
	if (d3dDebugController) {
		d3dDebugController->EnableDebugLayer();
		d3dDebugController->Release();
	}
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	// factory 생성
	CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(IDXGIFactory4), (void**)&_factory);

	// 어댑터(그래픽카드)
	IDXGIAdapter1* adapter{};
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(i, &adapter); ++i) {
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;	// 렌더링 드라이버라면?
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&_device))) break;	// direct12 디바이스 생성
	}
	if (!adapter) {	// 12를 지원하는 디바이스를 생성할 수 없으면 WARP 디바이스 생성
		_factory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), (void**)&adapter);
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&_device);
	}

	// 디바이스가 지원하는 다중 샘플 품질 수준 확인
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaAQualityLevels;
	msaaAQualityLevels.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	msaaAQualityLevels.SampleCount = 4;	// Msaa4x 다중 샘플링
	msaaAQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaAQualityLevels.NumQualityLevels = 0;
	_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaAQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	msaa4x_quality_level = msaaAQualityLevels.NumQualityLevels;

	// 품질 수준이 1보다 크면 다중 샘플링 활성화
	msaa4x_enabled = (msaa4x_quality_level > 1) ? true : false;

	// 동기화를 위한 fence 생성
	result = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&fence);
	for (int i = 0; i < swap_chain_buffer_num; ++i)
		fence_value[i] = 0;

	// 펜스와 동기화를 위한 이벤트 객체 생성. signal 시 이벤트 값을 자동적으로 FALSE가 되도록 생성
	/*
	HANDLE CreateEvent(
	LPSECURITY_ATTRIBUTES lpEventAttributes, // 보안 속성
	BOOL bManualReset,                      // 수동/자동 리셋 여부
	BOOL bInitialState,                     // 초기 상태 (신호/비신호)
	LPCSTR lpName                           // 이벤트 객체 이름
	);
	*/
	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 뷰포트를 윈도우 클라이언트 전체 영역으로 설정
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(ScreenWidth);
	viewport.Height = static_cast<float>(ScreenHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// 씨저 사각형을 클라이언트 영역 전체로 설정
	scissor_rect = { 0, 0, ScreenWidth, ScreenHeight };

	if (adapter) adapter->Release();
}

void GameFramework::CreateSwapChine()
{
	RECT rc;
	GetClientRect(_hwnd, &rc);

	// 따라하기05
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = swap_chain_buffer_num;
	swapChainDesc.BufferDesc.Width = ScreenWidth;
	swapChainDesc.BufferDesc.Height = ScreenHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = _hwnd;
	swapChainDesc.SampleDesc.Count = (msaa4x_enabled) ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = (msaa4x_enabled) ? (msaa4x_quality_level - 1) : 0;
	swapChainDesc.Windowed = TRUE;
	// 전체 화면 모드에서 바탕화면의 해상도를 후면 버퍼의 크기에 맞게 변경
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	_factory->CreateSwapChain(_commandQueue.Get(), &swapChainDesc, (IDXGISwapChain**)_swapChain.GetAddressOf());
	swap_chain_buffer_index = _swapChain->GetCurrentBackBufferIndex();
	_factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER);	// alt+enter에 응답하지 않게 설정
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE 
	CreateRenderTargetViews();
#endif // 
}

void GameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// GPU 계산이 오래 걸릴 때 time out 실행 후 다음 명형 실행
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// GPU가 모든 명령 직접 실행

	// 직접 명령 큐 생성
	_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&_commandQueue);

	// 할당자 생성
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&_commandAllocator);

	//리스트 생성
	_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)_commandList.GetAddressOf());

	// 리스트가 생성되면 Open상태가 됨
	_commandList->Close();
}

void GameFramework::CreateRtvAndDsvHeaps()
{
	// 렌더 타겟 서술자 힙(서술자 개수 = 스왑체인 버퍼의 개수) 생성
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = swap_chain_buffer_num;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;

	_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&rtv_descriptor_heap);

	// 렌더 타겟 서술자 힙 원소의 크기 저장
	rtv_increment_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 깊이-스탠실 서술자 힙(1개) 생성
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&dsv_descriptor_heap);

	// 원소 크기 저장
	dsv_increment_size = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void GameFramework::CreateRenderTargetViews()
{
	// 스왑체인의 각 후면 버퍼에 대한 렌더 타겟 뷰 생성
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < swap_chain_buffer_num; ++i) {
		_swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&render_target_buffers[i]);
		_device->CreateRenderTargetView(render_target_buffers[i].Get(), NULL, rtvCPUDescriptorHandle);
		rtvCPUDescriptorHandle.ptr += rtv_increment_size;
	}
}

void GameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = ScreenWidth;
	resourceDesc.Height = ScreenHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = (msaa4x_enabled) ? 4 : 1;
	resourceDesc.SampleDesc.Quality = (msaa4x_enabled) ? (msaa4x_quality_level - 1) : 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	// 버퍼 생성
	_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&depth_stencil_buffer);

	// 뷰 생성
	// 힙 시작 핸들 값
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDesctiptorHandle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	_device->CreateDepthStencilView(depth_stencil_buffer.Get(), NULL, dsvCPUDesctiptorHandle);
}

void GameFramework::WaitForGpuComplete()
{
	// CPU 펜스 값 증가
	const UINT64 fenceValue = ++fence_value[swap_chain_buffer_index];
	// GPU가 펜스 값을 설정하는 명령을 명령어 큐에 추가
	_commandQueue->Signal(fence.Get(), fenceValue);
	// GPU 펜스 값이 CPU 펜스 값보다 작으면 계속 이벤트를 기다림
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fence_event);
		::WaitForSingleObject(fence_event, INFINITE);
	}
}

void GameFramework::MoveToNextFrame()
{
	swap_chain_buffer_index = _swapChain->GetCurrentBackBufferIndex();

	UINT64 fenceValue = ++fence_value[swap_chain_buffer_index];
	_commandQueue->Signal(fence.Get(), fenceValue);
	if (fence->GetCompletedValue() < fenceValue) {
		fence->SetEventOnCompletion(fenceValue, fence_event);
		WaitForSingleObject(fence_event, INFINITE);
	}
}