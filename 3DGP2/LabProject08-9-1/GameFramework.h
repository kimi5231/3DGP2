#pragma once

// 화면 해상도
// Frame Buffer: GPU가 렌더링한 결과를 저장하는 메모리 공간
#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480

#include "Timer.h"
#include "Player.h"
#include "Scene.h"

// 렌더링 모드 구분용 매크로

// 색상만 출력하는 기본 렌더링
#define DRAW_SCENE_COLOR				'S'
// 텍스쳐를 적용한 렌더링
#define DRAW_SCENE_TEXTURE				'T'
// 조명 계산이 포함된 렌더링
#define DRAW_SCENE_LIGHTING				'L'
// 노멀 벡터 렌더링
#define DRAW_SCENE_NORMAL				'N'
// Z 버퍼값 렌더링
// Z 버퍼값: GPU가 픽셀의 깊이를 저장하는 버퍼 메모리
#define DRAW_SCENE_Z_DEPTH				'Z'
// 깊이값 렌더링
// 깊이값: 카메라 기준으로 물체까지의 거리값
#define DRAW_SCENE_DEPTH				'D'

// 프레임워크를 표현하는 클래스
class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	// Init
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	// 소멸자에서 할 일
	void OnDestroy();

	// SwapChain, Device, CommandQueue/List 생성
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	// RTV, DSV DescriptorHeap 생성
	void CreateRtvAndDsvDescriptorHeaps();

	// RTV, DSV 생성
	void CreateSwapChainRenderTargetViews();
	void CreateDepthStencilView();

	// 
	void ChangeSwapChainState();

	// Object 생성, 삭제
    void BuildObjects();
    void ReleaseObjects();

    void ProcessInput();
    void AnimateObjects();
    void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	// 프로그램 인스턴스 핸들
	HINSTANCE						m_hInstance;
	// 윈도우 창 핸들
	HWND							m_hWnd; 

	// 윈도우 창 사이즈
	int								m_nWndClientWidth;
	int								m_nWndClientHeight;
        
	// SwapChain, Device를 사용하기 위한 객체
	IDXGIFactory4					*m_pdxgiFactory = NULL;
	// 렌더링을 위한 BackBuffer, FrontBuffer를 교체해주는 객체
	IDXGISwapChain3					*m_pdxgiSwapChain = NULL;
	// GPU와 직접 통신하는 객체
	ID3D12Device					*m_pd3dDevice = NULL;

	// 계단 현상 관련 변수
	// MSAA 사용 여부
	bool							m_bMsaa4xEnable = false;
	// MSAA 품질 레벨
	UINT							m_nMsaa4xQualityLevels = 0;

	// SwapChine BackBuffer 수
	static const UINT				m_nSwapChainBuffers = 2;
	// 현재 사용 중인 SwapChain의 BackBuffer 위치를 기록하는 변수
	UINT							m_nSwapChainBufferIndex = 0;

	// SwapChin의 내부 BackBuffer를 참조할 배열
	ID3D12Resource					*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	
	// GPU가 사용할 RTV를 담아둘 DescriptorHeap 
	// RTV: GPU가 SwapChain BackBuffer에 접근하기 위해 필요한 View
	ID3D12DescriptorHeap			*m_pd3dRtvDescriptorHeap = NULL;
	// SwapChain마다 연결할 DescriptorHeap에 있는 RTV 핸들의 모음
	D3D12_CPU_DESCRIPTOR_HANDLE		m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBuffers];

	// DSV와 연결될 Buffer
	ID3D12Resource					*m_pd3dDepthStencilBuffer = NULL;
	// GPU가 사용할 DSV를 담아둘 DescriptorHeap
	// DSV: GPU가 DepthStencilBuffer에 접근하기 위해 필요한 View
	ID3D12DescriptorHeap			*m_pd3dDsvDescriptorHeap = NULL;
	// DescriptorHeap에 있는 DSV 핸들
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dDsvDescriptorCPUHandle;

	// CommandList가 명령을 기록할 공간을 관리하는 객체
	ID3D12CommandAllocator			*m_pd3dCommandAllocator = NULL;
	// CommandList 자체를 쌓아두는 객체
	// GPU가 CommandQueue에서 CommandList를 빼감 
	ID3D12CommandQueue				*m_pd3dCommandQueue = NULL;
	// CommandAllocator가 할당한 공간에 명령을 기록하는 객체
	ID3D12GraphicsCommandList		*m_pd3dCommandList = NULL;

	// CPU, GPU 동기화용 객체
	ID3D12Fence						*m_pd3dFence = NULL;
	// SwapChain BackBuffer마다 사용할 Fence들
	UINT64							m_nFenceValues[m_nSwapChainBuffers];
	// CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들
	HANDLE							m_hFenceEvent;

	CGameTimer						m_GameTimer;

	CScene							*m_pScene = NULL;
	CPlayer							*m_pPlayer = NULL;
	CCamera							*m_pCamera = NULL;

	// 후처리 Shader
	CPostProcessingShader			*m_pPostProcessingShader = NULL;

	// 렌더링 모드
	int	m_nDrawOption = DRAW_SCENE_COLOR;

	// 마우스 이전 좌표
	POINT m_ptOldCursorPos;

	// FrameRate를 문자열로 저장하기 위한 버퍼
	_TCHAR	m_pszFrameRate[50];
};