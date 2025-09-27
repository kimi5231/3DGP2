//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	// Framework 멤버 변수 초기화
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) 
		m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) 
		m_nFenceValues[i] = 0;

	// 윈도우 창 사이즈 설정
	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	// 윈도우 창 제목에 사용할 문자열 준비
	_tcscpy_s(m_pszFrameRate, _T("LabProject ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	// 프로그램 인스턴스, 윈도우 창 핸들 설정
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	// Deivce, CommandQueue/List, SwapChain 생성
	// 순서를 지켜야 함
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();

	// RTV, DSV DescriptorHeap 생성
	// 순서 상관없음. DescriptorHeap을 사용하기 전에만 하면 됨.
	CreateRtvAndDsvDescriptorHeaps();

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	// RTV 생성
	CreateSwapChainRenderTargetViews();
#endif
	// DSV 생성
	CreateDepthStencilView();

	// Object 생성
	BuildObjects();

	return(true);
}

//#define _WITH_SWAPCHAIN

void CGameFramework::CreateSwapChain()
{
// 윈도우 핸들 기반 SwapChain 생성
// SwapChine을 윈도우 창에 직접 연결
#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	// 생성할 SwapChain의 정보를 담을 객체
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	// 메모리를 0으로 초기화
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	// SwapChain BackBuffer 크기
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	// SwapChain BackBuffer 포맷
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// MSAA 샘플 수 (하나의 픽셀을 위해 샘플링하는 수)
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable)? 4 : 1;
	// MSAA 품질 레벨
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable)? (m_nMsaa4xQualityLevels - 1): 0;
	// SwapChain BackBuffer 사용 용도 (RenderTarget으로 사용)
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// SwapChain BackBuffer 수
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	// 윈도우 창 사이즈에 맞춰 BackBuffer 스케일링을 어떻게 할 지 (스케일링X)
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	// Buffer 교환 방식 (최신 방식)
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// BackBuffer에서 알파 채널을 어떻게 할 지 (알파 채널 사용 여부 명시X)
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

// 전체화면 전환 기능 설정
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	// 전체화면 전환 기능X (창 크기만 변경 가능)
	dxgiSwapChainDesc.Flags = 0;
#else
	// 전체화면 전환 기능 추가
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	// SwapChain 전체화면 모드 전환 시, 디스플레이 설정 정보를 담을 객체
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	// 메모리 0으로 초기화
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	// 화면 재생률 (1초에 60번)
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	// 스캔라인 순서 (GPU가 알아서 결정)
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// 스케일링 방식 (명시적으로 지정X)
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// 초기 모드 (창모드)
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	// 윈도우 핸들 기반 SwapChain 생성
	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	// 생성할 SwapChain의 정보를 담을 객체
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	// 메모리를 0으로 초기화
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	// SwapChain BackBuffer 수
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	// SwapChain BackBuffer 크기
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	// SwapChain BackBuffer 포맷
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// 화면 재생률 (1초에 60번)
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	// SwapChain BackBuffer 사용 용도 (RenderTarget으로 사용)
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Buffer 교환 방식 (최신 방식)
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// 출력 대상 윈도우 핸들
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	// MSAA 샘플 수 (하나의 픽셀을 위해 샘플링하는 수)
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable)? 4 : 1;
	// MSAA 품질 레벨
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable)? (m_nMsaa4xQualityLevels - 1): 0;
	// 초기 모드 (창모드)
	dxgiSwapChainDesc.Windowed = TRUE;

// 전체화면 전환 기능 설정
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	// 전체화면 전환 기능X (창 크기만 변경 가능)
	dxgiSwapChainDesc.Flags = 0;
#else
	// 전체화면 전환 기능 추가
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
#endif

	// SwapChain 생성 
	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif

	// Alt+Enter 전체화면 전환 막기
	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	// 현재 GPU가 렌더링할 수 있는 BackBuffer Index 얻어오기
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	// Factory Flag
	// Factory의 어떤 기능을 켤지를 지정하는 용도
	UINT nDXGIFactoryFlags = 0;

// Debug 빌드일 때, GPU 관련 오류를 콘솔에 출력
#if defined(_DEBUG)
	// Debug Layer을 제어하는 인터페이스 객체 생성
	ID3D12Debug *pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pd3dDebugController);
	if (pd3dDebugController)
	{
		// Debug Layer 활성화
		// D3D12 API 관련 오류 콘솔 출력
		pd3dDebugController->EnableDebugLayer();
		// 기능 활성화로 역할이 끝났으니 참조 해제
		pd3dDebugController->Release();
	}
	// Factory Flag에 Debug 기능 추가
	// DXGI API 관련 오류 콘솔 출력
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// Factory 생성
	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

	// GPU 하드웨어를 식별하고 선택하는 객체
	IDXGIAdapter1 *pd3dAdapter = NULL;

	// GPU 하드웨어를 식별하고, 이를 기반으로 Device 생성
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		// GPU의 정보를 담을 객체
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		// GPU 정보 조회
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);

		// 소프트웨어 GPU는 제외
		// 하드웨어 GPU만 사용하기 위해
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;
		// Adapter를 기반으로 Device 생성 시도
		// 성공 시, 루프 중단
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice)))
			break;
	}

	// Device 생성 실패, 소프트웨어 GPU로 Divice 생성 시도
	if (!m_pd3dDevice)
	{
		hResult = m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	// Device 생성 실패, 프로그램 종료
	if (!m_pd3dDevice)
	{
		MessageBox(NULL, L"Direct3D 12 Device Cannot be Created.", L"Error", MB_OK);
		::PostQuitMessage(0);
		return;
	}

	// GPU가 지원 가능한 MSAA 품질 정보를 담을 객체
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	// 확인할 RenderTarget의 포맷 지정
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// 확인할 샘플 수 지정 (MSAAx4)
	d3dMsaaQualityLevels.SampleCount = 4;
	// 확인 방식을 기본으로 설정
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	// 지원 가능한 MSAA 품질 레벨의 수를 저장할 변수 초기화
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	// 지원 가능한 조건에 맞는 MSAA 품질 레벨의 수 얻어오고 저장
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	// MSAA 품질 레벨 수에 따라 MSAA 사용 여부 설정
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1)? true : false;

	// Fence 객체 생성
	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	// SwapChain BackBuffer마다 목표 Fence값 지정
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
		m_nFenceValues[i] = 1;
	//  CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들 생성
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 전역변수
	// RTV offset값
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// CBV, SRV offset값
	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Adapter에 역할이 끝났으므로, 참조 해제
	if (pd3dAdapter)
		pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	// 생성할 CommandQueue의 정보를 담을 객체
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	// 메모리 0으로 초기화
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	// CommandQueue의 추가 기능 설정 (추가 기능 없음)
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// CommandQueue의 종류 (기본 타입)
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// CommandQueue 생성
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);
	// CommandAllocator, CommandList 생성 (CommandQueue와 같은 타입으로 생성해야 함)
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);
	// CommandList 닫기 (생성할 때는 기본적으로 열려 있음)
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	// 생성할 DescriptorHeap의 정보를 담을 객체
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	// 메모리 0으로 초기화
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	
	// RTV
	// DescriptorHeap에 넣을 Descriptor 수
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers + 5;
	// RTV DescriptorHeap으로 설정
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// 추가 모드 설정 (추가 모드 없음)
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// 어느 GPU에 연결할지 (기본)
	d3dDescriptorHeapDesc.NodeMask = 0;
	// RTV DescriptorHeap 생성
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);

	// DSV
	// DescriptorHeap에 넣을 Descriptor 수
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	// DSV DescriptorHeap으로 설정
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	// DSV DescriptorHeap 생성
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
}

void CGameFramework::CreateSwapChainRenderTargetViews()
{
	// 생성할 RTV의 정보를 담을 객체
	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	// RTV 포맷
	d3dRenderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// 2차원 View로 설정
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	// 텍스쳐의 몇 번째 밉맵을 RenderTarget으로 사용할지 지정
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	// RenderTarget으로 사용할 텍스쳐 평면 지정 (색상 정보)
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;

	// RTV DescriptorHeap에서 핸들 얻어오기
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		// SwapChain BackBuffer를 미리 만들어둔 배열에 연결
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dSwapChainBackBuffers[i]);
		// RTV를 생성하고 SwapChain BackBuffer를 참조하는 배열에 연결 및 DescriptorHeap에 등록
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], &d3dRenderTargetViewDesc, d3dRtvCPUDescriptorHandle);
		// SwapChain과 연결된 RTV 핸들 저장
		m_pd3dSwapChainBackBufferRTVCPUHandles[i] = d3dRtvCPUDescriptorHandle;
		// RTV offset만큼 이동
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	// 생성할 DSV의 정보를 담을 객체
	D3D12_RESOURCE_DESC d3dResourceDesc;
	// 2차원 View로 설정 
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// GPU 메모리 리소스 정렬 방식 (Direct3D가 알아서)
	d3dResourceDesc.Alignment = 0;
	// DSV 크기 지정
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	// 2D 텍스쳐: 배열의 크기 지정, 3D 텍스쳐: 3D 텍스쳐를 구성하는 슬라이스 수 지정
	d3dResourceDesc.DepthOrArraySize = 1;
	// 밉맵 레벨 지정
	d3dResourceDesc.MipLevels = 1;
	// DSV 포맷 
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DXGI_FORMAT_D32_FLOAT;
	// MSAA 샘플 수 
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable)? 4 : 1;
	// MSAA 품질 레벨
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable)? (m_nMsaa4xQualityLevels - 1): 0;
	// 텍스쳐의 메모리 레이아웃 (Direct3D가 알아서)
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	// 추가 모드 설정 (DSV를 Depth Stencil Buffer 사용)
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 리소스를 GPU 메모리 어느 Heap에 저장할 지에 대한 설정 정보를 담을 객체
	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	// 메모리 0으로 초기화
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	// Heap 종류 (GPU 전용 메모리)
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	// CPU 접근 방법 (Direct3D가 알아서)
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	// GPU 메모리 풀 중 어떤 걸 우선 사용하지 지정 (Direct3D가 알아서)
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	// 어떤 GPU를 사용할 지 지정
	d3dHeapProperties.CreationNodeMask = 1;
	// 어떤 GPU가 접근할 수 있는지 지정
	d3dHeapProperties.VisibleNodeMask = 1;
	
	// DSV 초기화할 때 사용할 정보를 담을 객체
	D3D12_CLEAR_VALUE d3dClearValue;
	// 초기화할 리소스 포맷 (DSV)
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DXGI_FORMAT_D32_FLOAT;
	// 초기화로 설정할 Depth, Stencil 값
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	// GPU Heap에 리소스 생성 후 커밋
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	// DSV DescriptorHeap 핸들 얻어오기
	m_d3dDsvDescriptorCPUHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// DSV를 생성하고 DSV DescriptorHeap에 등록
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, m_d3dDsvDescriptorCPUHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	// GPU의 작업이 완료될 때까지 대기
	WaitForGpuComplete();

	// SwapChain의 전체화면 상태를 확인하고, 현재 상태의 반대되는 상태로 설정
	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	// 
	DXGI_MODE_DESC dxgiTargetParameters;
	// BackBuffer 포맷
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// BackBuffer 크기
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	// 화면 재생률 (1초 60번)
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	// 
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	// 기존에 참조하던 BackBuffer 정보 삭제
	for (int i = 0; i < m_nSwapChainBuffers; i++)
		if (m_ppd3dSwapChainBackBuffers[i])
			m_ppd3dSwapChainBackBuffers[i]->Release();

#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
#else
	// SwapChain 정보를 얻어와 BackBuffer 사이즈 재조정
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
#endif

	// 
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	// 바뀐 SwapChain에 맞춰 RTV 재생성
	CreateSwapChainRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	// 마우스 
	if (m_pScene)
		m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
	
	switch (nMessageID)
	{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			::SetCapture(hWnd);
			// 마우스 좌표 얻어오기
			::GetCursorPos(&m_ptOldCursorPos);
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			::ReleaseCapture();
			break;
		case WM_MOUSEMOVE:
			break;
		default:
			break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene)
		m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	
	switch (nMessageID)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					::PostQuitMessage(0);
					break;
				case VK_RETURN:
					break;
				case VK_F1:
				case VK_F2:
				case VK_F3:
					m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
					break;
				case VK_F9:
					ChangeSwapChainState();
					break;
				case 'S': //83
				case 'T': //84
				case 'D': //68
				case 'Z': //90
				case 'N': //78
				case 'L': //76
				{
					m_nDrawOption = (int)wParam;
					break;
				}
				default:
					break;
			}
			break;
		default:
			break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam)== WA_INACTIVE)
				m_GameTimer.Stop();
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
			break;
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
	// Object 삭제
    ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer)m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap)m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++)if (m_ppd3dSwapChainBackBuffers[i])m_ppd3dSwapChainBackBuffers[i]->Release();

	if (m_pd3dRtvDescriptorHeap)m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator)m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue)m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList)m_pd3dCommandList->Release();

	if (m_pd3dFence)m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain)m_pdxgiSwapChain->Release();
    if (m_pd3dDevice)m_pd3dDevice->Release();
	if (m_pdxgiFactory)m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1	*pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void **)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CGameFramework::BuildObjects()
{
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);
	
	m_pScene = new CScene();
	m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);

	m_pScene->m_pPlayer = m_pPlayer = new CAirplanePlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), NULL, 1);
	m_pCamera = m_pPlayer->GetCamera();

	m_pPostProcessingShader = new CTextureToFullScreenShader();
	m_pPostProcessingShader->CreateShader(m_pd3dDevice, m_pScene->GetGraphicsRootSignature(), 1, NULL, DXGI_FORMAT_D24_UNORM_S8_UINT/*DXGI_FORMAT_D32_FLOAT*/);
	m_pPostProcessingShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, &m_nDrawOption);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (::gnRtvDescriptorIncrementSize * m_nSwapChainBuffers);

	DXGI_FORMAT pdxgiResourceFormats[4] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_FLOAT };
	m_pPostProcessingShader->CreateResourcesAndRtvsSrvs(m_pd3dDevice, m_pd3dCommandList, 4, pdxgiResourceFormats, d3dRtvCPUDescriptorHandle); //SRV to (Render Targets) + (Depth Buffer)

	D3D12_GPU_DESCRIPTOR_HANDLE d3dDsvGPUDescriptorHandle = CScene::CreateShaderResourceView(m_pd3dDevice, m_pd3dDepthStencilBuffer, DXGI_FORMAT_R24_UNORM_X8_TYPELESS /*DXGI_FORMAT_R32_FLOAT*/);

	m_pd3dCommandList->Close();

	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

	if (m_pScene)m_pScene->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	// Player 삭제
	if (m_pPlayer)
		delete m_pPlayer;

	// Scene, Scene에 있는 Object 삭제
	if (m_pScene)
		m_pScene->ReleaseObjects();
	if (m_pScene)
		delete m_pScene;

	// 후처리 Shader, 후처리 Shader에 있는 Object 삭제
	if (m_pPostProcessingShader)
		m_pPostProcessingShader->ReleaseObjects();
	if (m_pPostProcessingShader)
		m_pPostProcessingShader->Release();
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;

	if (GetKeyboardState(pKeysBuffer)&& m_pScene)
		bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;
		if (pKeysBuffer[VK_UP] & 0xF0)dwDirection |= DIR_FORWARD;
		if (pKeysBuffer[VK_DOWN] & 0xF0)dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[VK_LEFT] & 0xF0)dwDirection |= DIR_LEFT;
		if (pKeysBuffer[VK_RIGHT] & 0xF0)dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0)dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0)dwDirection |= DIR_DOWN;

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture()== m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x)/ 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y)/ 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0)|| (cxDelta != 0.0f)|| (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection)m_pPlayer->Move(dwDirection, 50.0f * m_GameTimer.GetTimeElapsed(), true);
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::AnimateObjects()
{
	if (m_pScene)
		m_pScene->AnimateObjects(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue()< nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue()< nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{    
	m_GameTimer.Tick(0.0f);
	
	ProcessInput();

    AnimateObjects();

	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_pScene->OnPrepareRender(m_pd3dCommandList, m_pCamera);

	if (m_nDrawOption == DRAW_SCENE_COLOR)//'S'
	{
		m_pd3dCommandList->ClearDepthStencilView(m_d3dDsvDescriptorCPUHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

		m_pPostProcessingShader->OnPrepareRenderTarget(m_pd3dCommandList, 1, &m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], &m_d3dDsvDescriptorCPUHandle);

		m_pScene->Render(m_pd3dCommandList, m_pCamera);

		m_pPlayer->Render(m_pd3dCommandList, m_pCamera);

		m_pPostProcessingShader->OnPostRenderTarget(m_pd3dCommandList);
	}
	else
	{
		m_pd3dCommandList->OMSetRenderTargets(1, &m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], TRUE, NULL);

		m_pPostProcessingShader->Render(m_pd3dCommandList, m_pCamera, &m_nDrawOption);
	}

	::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	hResult = m_pd3dCommandList->Close();

	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

	MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}