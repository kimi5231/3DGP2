#include "pch.h"
#include "GameFramework.h"
#include "TitleScene.h"
#include "GameScene.h"

GameFramework::GameFramework(HWND hwnd)
{
	_hwnd = hwnd;
	
	// Device -> CommandList -> SwapChain 순서 지켜야 함
	CreateDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();

	CreateRtvDsvDescriptorHeaps();
	CreateRenderTargetViews();
	CreateDepthStencilView();

	_commandList->Reset(_commandAllocator.Get(), NULL);

	_scene = new TitleScene(_device, _commandList);

	_commandList->Close();
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, commandLists);

	WaitForGpuComplete();

	if (_scene) 
		_scene->ReleaseUploadBuffers();
}

GameFramework::~GameFramework()
{
}

void GameFramework::Update()
{
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
	resourceBarrier.Transition.pResource = _swapChainBackBuffers[_swapChainBufferIndex].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &resourceBarrier);

	// 현재 렌더 타겟에 해당하는 서술자의 CPU 주소(핸들) 값을 계산
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDesciptorHandle = _rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvCPUDesciptorHandle.ptr += (_swapChainBufferIndex * _rtvDescriptorIncrementSize);

	// 깊이-스텐실 서술자의 CPU 주소를 계산한다.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = _dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 렌더 타겟 뷰와 깊이 스텐실 뷰를 출력 병합 단계(OM)에 연결
	_commandList->OMSetRenderTargets(1, &rtvCPUDesciptorHandle, FALSE, &dsvCPUDescriptorHandle);

	// 원하는 색상으로 렌더 타겟 지우기
	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	_commandList->ClearRenderTargetView(rtvCPUDesciptorHandle, clearColor, 0, NULL);

	// 원하는 값으로 깊이 스텐실 지우기
	_commandList->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, NULL);

	ProcessInput();

	if (_scene) 
		_scene->Render(_commandList.Get());

	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &resourceBarrier);

	_commandList->Close();
	ID3D12CommandList* commandLists[]{ _commandList.Get() };
	_commandQueue->ExecuteCommandLists(1, commandLists);
	WaitForGpuComplete();

	_swapChain->Present(0, 0);
	
	MoveToNextFrame();
}

void GameFramework::Render()
{
	
}

void GameFramework::CreateDevice()
{
	// Factory의 어떤 기능을 켤지를 지정하는 용도
	UINT factoryFlag;

	// Debug 빌드일 때, GPU 관련 오류를 콘솔에 출력
#if defined(_DEBUG)
	// Debug Layer을 제어하는 인터페이스 객체 생성
	ID3D12Debug* debugController;
	D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&debugController);
	if (debugController) 
	{
		// Debug Layer 활성화
		// D3D12 API 관련 오류 콘솔 출력
		debugController->EnableDebugLayer();
		// 기능 활성화로 역할이 끝났으니 참조 해제
		debugController->Release();
	}
	// Factory Flag에 Debug 기능 추가
	// DXGI API 관련 오류 콘솔 출력
	factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// Factory 생성
	CreateDXGIFactory2(factoryFlag, __uuidof(IDXGIFactory4), (void**)&_factory);

	// GPU 하드웨어를 식별하고 선택하는 객체
	IDXGIAdapter1* adapter;

	// GPU 하드웨어를 식별하고, 이를 기반으로 Device 생성
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(i, &adapter); ++i)
	{
		// GPU의 정보를 담을 객체
		DXGI_ADAPTER_DESC1 adapterDesc;
		// GPU 정보 조회
		adapter->GetDesc1(&adapterDesc);

		// 하드웨어 GPU만 사용하기 위해 소프트웨어 GPU는 제외
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) 
			continue;
		// Adapter를 기반으로 Device 생성 시도, 성공 시 루프 중단
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&_device))) 
			break;
	}

	// Device 생성 실패, 소프트웨어 GPU로 Divice 생성 시도
	if (!adapter) 
	{
		_factory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), (void**)&adapter);
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&_device);
	}

	// Device 생성 실패, 프로그램 종료
	if (!_device)
	{
		MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	// GPU가 지원 가능한 MSAA 품질 정보를 담을 객체
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevel;
	// 확인할 RenderTarget의 포맷 지정
	msaaQualityLevel.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	// 확인할 샘플 수 지정 (MSAAx4)
	msaaQualityLevel.SampleCount = 4;
	// 확인 방식을 기본으로 설정
	msaaQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	// 지원 가능한 MSAA 품질 레벨의 수를 저장할 변수 초기화
	msaaQualityLevel.NumQualityLevels = 0;
	// 지원 가능한 조건에 맞는 MSAA 품질 레벨의 수 얻어오고 저장
	_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevel, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	_msaa4xQualityLevel = msaaQualityLevel.NumQualityLevels;
	// MSAA 품질 레벨 수에 따라 MSAA 사용 여부 설정
	_msaa4xEnable = (_msaa4xQualityLevel > 1) ? true : false;

	// Fence 객체 생성
	_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&_fence);
	// SwapChain BackBuffer마다 목표 Fence값 지정
	for (int i = 0; i < _swapChainBufferCount; ++i)
		_fenceValues[i] = 0;
	//  CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들 생성
	_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	{
		// 뷰포트를 윈도우 클라이언트 전체 영역으로 설정
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<float>(ScreenWidth);
		viewport.Height = static_cast<float>(ScreenHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// 씨저 사각형을 클라이언트 영역 전체로 설정
		scissor_rect = { 0, 0, ScreenWidth, ScreenHeight };
	}

	// Adapter의 역할이 끝났으므로 참조 해제
	if (adapter) 
		adapter->Release();
}

void GameFramework::CreateCommandQueueAndList()
{
	// 생성할 CommandQueue의 정보를 담을 객체
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	// 메모리 0으로 초기화
	::ZeroMemory(&commandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	// CommandQueue의 추가 기능 설정 (추가 기능 없음)
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// CommandQueue의 종류 (기본 타입)
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// CommandQueue 생성
	_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&_commandQueue);

	// CommandAllocator, CommandList 생성 (CommandQueue와 같은 타입으로 생성해야 함)
	_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&_commandAllocator);
	_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)_commandList.GetAddressOf());
	// CommandList 닫기 (생성할 때는 기본적으로 열려 있음)
	_commandList->Close();
}

void GameFramework::CreateSwapChain()
{
	// 생성할 SwapChain의 정보를 담을 객체
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	// 메모리를 0으로 초기화
	::ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	// SwapChain BackBuffer 수
	swapChainDesc.BufferCount = _swapChainBufferCount;
	// SwapChain BackBuffer 크기
	swapChainDesc.BufferDesc.Width = ScreenWidth;
	swapChainDesc.BufferDesc.Height = ScreenHeight;
	// SwapChain BackBuffer 포맷
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// 화면 재생률 (1초에 60번)
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	// SwapChain BackBuffer 사용 용도 (RenderTarget으로 사용)
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Buffer 교환 방식 (최신 방식)
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 출력 대상 윈도우 핸들
	swapChainDesc.OutputWindow = _hwnd;
	// MSAA 샘플 수 (하나의 픽셀을 위해 샘플링하는 수)
	swapChainDesc.SampleDesc.Count = (_msaa4xEnable) ? 4 : 1;
	// MSAA 품질 레벨
	swapChainDesc.SampleDesc.Quality = (_msaa4xEnable) ? (_msaa4xQualityLevel - 1) : 0;
	// 초기 모드 (창모드)
	swapChainDesc.Windowed = TRUE;
	// 전체화면 전환 기능 추가
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// SwapChain 생성
	_factory->CreateSwapChain(_commandQueue.Get(), &swapChainDesc, (IDXGISwapChain**)_swapChain.GetAddressOf());
	// 현재 GPU가 렌더링할 수 있는 BackBuffer Index 얻어오기
	_swapChainBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	// Alt+Enter 전체화면 전환 막기
	_factory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER);
}

void GameFramework::CreateRtvDsvDescriptorHeaps()
{
	// 생성할 DescriptorHeap의 정보를 담을 객체
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	// 메모리 0으로 초기화
	::ZeroMemory(&descriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	
	// RTV
	// DescriptorHeap에 넣을 Descriptor 수
	descriptorHeapDesc.NumDescriptors = _swapChainBufferCount;
	// RTV DescriptorHeap으로 설정
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// 추가 모드 설정 (추가 모드 없음)
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// 어느 GPU에 연결할지 (기본)
	descriptorHeapDesc.NodeMask = 0;
	// RTV DescriptorHeap 생성
	_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&_rtvDescriptorHeap);
	// RTV 건너뛸 값 설정
	_rtvDescriptorIncrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// DSV
	// DescriptorHeap에 넣을 Descriptor 수
	descriptorHeapDesc.NumDescriptors = 1;
	// DSV DescriptorHeap으로 설정
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	// DSV DescriptorHeap 생성
	_device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&_dsvDescriptorHeap);
}

void GameFramework::CreateRenderTargetViews()
{
	// RTV DescriptorHeap에서 핸들 얻어오기
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorCPUHandle = _rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < _swapChainBufferCount; ++i) 
	{
		// SwapChain BackBuffer를 미리 만들어둔 배열에 연결
		_swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&_swapChainBackBuffers[i]);
		// RTV를 생성하고 SwapChain BackBuffer를 참조하는 배열에 연결 및 DescriptorHeap에 등록
		_device->CreateRenderTargetView(_swapChainBackBuffers[i].Get(), NULL, rtvDescriptorCPUHandle);
		// RTV 크기만큼 이동
		rtvDescriptorCPUHandle.ptr += _rtvDescriptorIncrementSize;
	}
}

void GameFramework::CreateDepthStencilView()
{
	// 생성할 DSV의 정보를 담을 객체
	D3D12_RESOURCE_DESC resourceDesc;
	// 2차원 View로 설정
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// GPU 메모리 리소스 정렬 방식 (Direct3D가 알아서)
	resourceDesc.Alignment = 0;
	// DSV 크기 지정
	resourceDesc.Width = ScreenWidth;
	resourceDesc.Height = ScreenHeight;
	// 2D 텍스쳐: 배열의 크기 지정, 3D 텍스쳐: 3D 텍스쳐를 구성하는 슬라이스 수 지정
	resourceDesc.DepthOrArraySize = 1;
	// Mipmap 레벨 지정
	resourceDesc.MipLevels = 1;
	// DSV 포맷 
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// MSAA 샘플 수
	resourceDesc.SampleDesc.Count = (_msaa4xEnable) ? 4 : 1;
	// MSAA 품질 레벨
	resourceDesc.SampleDesc.Quality = (_msaa4xEnable) ? (_msaa4xQualityLevel - 1) : 0;
	// 텍스쳐의 메모리 레이아웃 (Direct3D가 알아서)
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	// 추가 모드 설정 (DSV를 Depth Stencil Buffer 사용)
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 리소스를 GPU 메모리 어느 Heap에 저장할 지에 대한 설정 정보를 담을 객체
	D3D12_HEAP_PROPERTIES heapProperties;
	// 메모리 0으로 초기화
	::ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	// Heap 종류 (GPU 전용 메모리)
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	// CPU 접근 방법 (Direct3D가 알아서)
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	// GPU 메모리 풀 중 어떤 걸 우선 사용하지 지정 (Direct3D가 알아서)
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	// 어떤 GPU를 사용할 지 지정
	heapProperties.CreationNodeMask = 1;
	// 어떤 GPU가 접근할 수 있는지 지정
	heapProperties.VisibleNodeMask = 1;

	// DSV 초기화할 때 사용할 정보를 담을 객체
	D3D12_CLEAR_VALUE clearValue;
	// 초기화할 리소스 포맷 (DSV)
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// 초기화로 설정할 Depth, Stencil 값
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// GPU Heap에 리소스 생성 후 커밋
	_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&_depthStencilBuffer);

	// DSV DescriptorHeap 핸들 얻어오기
	D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorCPUHandle = _dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// DSV를 생성하고 DSV DescriptorHeap에 등록
	_device->CreateDepthStencilView(_depthStencilBuffer.Get(), NULL, dsvDescriptorCPUHandle);
}

void GameFramework::WaitForGpuComplete()
{
	// CPU 펜스 값 증가
	const UINT64 fenceValue = ++_fenceValues[_swapChainBufferIndex];
	// GPU가 펜스 값을 설정하는 명령을 명령어 큐에 추가
	_commandQueue->Signal(_fence.Get(), fenceValue);
	// GPU 펜스 값이 CPU 펜스 값보다 작으면 계속 이벤트를 기다림
	if (_fence->GetCompletedValue() < fenceValue) {
		_fence->SetEventOnCompletion(fenceValue, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void GameFramework::MoveToNextFrame()
{
	_swapChainBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	UINT64 fenceValue = ++_fenceValues[_swapChainBufferIndex];
	_commandQueue->Signal(_fence.Get(), fenceValue);
	if (_fence->GetCompletedValue() < fenceValue) {
		_fence->SetEventOnCompletion(fenceValue, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void GameFramework::ProcessInput()
{
	if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
	{
		POINT mousePos;
		GetCursorPos(&mousePos);
		ScreenToClient(_hwnd, &mousePos);

		float mPosX = (2.0f * mousePos.x / ScreenWidth) - 1.0f;
		float mPosY = 1.0f - (2.0f * mousePos.y / ScreenHeight);
	
		if (mPosX <= -0.4 && mPosX >= -0.9)
		{
			// Game Start Button
			if (mPosY <= 0.9 && mPosY >= 0.7)
				ChangeScene();
			
			// Game End Button
			if (mPosY <= 0.6 && mPosY >= 0.4)
				PostQuitMessage(0);
		}
	}
}

void GameFramework::ChangeScene()
{
	delete _scene;
	_scene = new GameScene(_device, _commandList);
}