#pragma once
class GameFramework
{
public:
	GameFramework(HWND hwnd);
	~GameFramework();

public:
	void Update();
	void Render();

private:
	void CreateDevice();
	void CreateSwapChine();
	void CreateCommandQueueAndList();

private:
	HWND _hwnd;

	// SwapChain, Device, Adapter를 사용하기 위한 객체
	IDXGIFactory4* m_pdxgiFactory = NULL;
	// 렌더링을 위한 BackBuffer, FrontBuffer를 교체해주는 객체
	IDXGISwapChain3* m_pdxgiSwapChain = NULL;
	// SwapChine BackBuffer 수
	static const UINT				m_nSwapChainBuffers = 2;
	// 현재 사용 중인 SwapChain의 BackBuffer 위치를 기록하는 변수
	UINT							m_nSwapChainBufferIndex = 0;
	// GPU와 직접 통신하는 객체
	ID3D12Device* m_pd3dDevice = NULL;
	// CommandList가 명령을 기록할 공간을 관리하는 객체
	ID3D12CommandAllocator* m_pd3dCommandAllocator = NULL;
	// CommandList 자체를 쌓아두는 객체
	// GPU가 CommandQueue에서 CommandList를 빼감 
	ID3D12CommandQueue* m_pd3dCommandQueue = NULL;
	// CommandAllocator가 할당한 공간에 명령을 기록하는 객체
	ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;

	// CPU, GPU 동기화용 객체
	ID3D12Fence* m_pd3dFence = NULL;
	// SwapChain BackBuffer마다의 목표 Fence값
	UINT64							m_nFenceValues[m_nSwapChainBuffers];
	// CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들
	HANDLE							m_hFenceEvent;

	// 계단 현상 관련 변수
	// MSAA 사용 여부
	bool							m_bMsaa4xEnable = false;
	// MSAA 품질 레벨의 수
	UINT							m_nMsaa4xQualityLevels = 0;

	// SwapChin의 내부 BackBuffer를 참조할 배열
	ID3D12Resource* m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
};