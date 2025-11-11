#include "pch.h"
#include "GameFramework.h"

GameFramework::GameFramework(HWND hwnd)
{
	_hwnd = hwnd;

	CreateDevice();
	CreateCommandQueueAndList();
	CreateSwapChine();
}

GameFramework::~GameFramework()
{
}

void GameFramework::Update()
{

}

void GameFramework::Render()
{
	m_pdxgiSwapChain->Present(0, 0);
}

void GameFramework::CreateDevice()
{
	HRESULT hResult;

	// Factory Flag
	// Factory의 어떤 기능을 켤지를 지정하는 용도
	UINT nDXGIFactoryFlags = 0;

	// Debug 빌드일 때, GPU 관련 오류를 콘솔에 출력
#if defined(_DEBUG)
	// Debug Layer을 제어하는 인터페이스 객체 생성
	ID3D12Debug* pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&pd3dDebugController);
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
	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);

	// GPU 하드웨어를 식별하고 선택하는 객체
	IDXGIAdapter1* pd3dAdapter = NULL;

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
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice)))
			break;
	}

	// Device 생성 실패, 소프트웨어 GPU로 Divice 생성 시도
	if (!m_pd3dDevice)
	{
		hResult = m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIAdapter1), (void**)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice);
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
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	// Fence 객체 생성
	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	// SwapChain BackBuffer마다 목표 Fence값 지정
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
		m_nFenceValues[i] = 1;
	//  CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들 생성
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// 전역변수
	// RTV offset값
	//::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// CBV, SRV offset값
	//::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Adapter에 역할이 끝났으므로, 참조 해제
	if (pd3dAdapter)
		pd3dAdapter->Release();
}

void GameFramework::CreateSwapChine()
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
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	// MSAA 품질 레벨
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
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
	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)&m_pdxgiSwapChain);
#else
	// 생성할 SwapChain의 정보를 담을 객체
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	// 메모리를 0으로 초기화
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	// SwapChain BackBuffer 수
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	// SwapChain BackBuffer 크기
	dxgiSwapChainDesc.BufferDesc.Width = ScreenWidth;
	dxgiSwapChainDesc.BufferDesc.Height = ScreenHeight;
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
	dxgiSwapChainDesc.OutputWindow = _hwnd;
	// MSAA 샘플 수 (하나의 픽셀을 위해 샘플링하는 수)
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	// MSAA 품질 레벨
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
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
	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain**)&m_pdxgiSwapChain);
#endif

	// Alt+Enter 전체화면 전환 막기
	hResult = m_pdxgiFactory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_ALT_ENTER);
	// 현재 GPU가 렌더링할 수 있는 BackBuffer Index 얻어오기
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void GameFramework::CreateCommandQueueAndList()
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
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);
	// CommandAllocator, CommandList 생성 (CommandQueue와 같은 타입으로 생성해야 함)
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_pd3dCommandAllocator);
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	// CommandList 닫기 (생성할 때는 기본적으로 열려 있음)
	hResult = m_pd3dCommandList->Close();
}