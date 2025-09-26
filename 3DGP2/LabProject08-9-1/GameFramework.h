#pragma once

// ȭ�� �ػ�
// Frame Buffer: GPU�� �������� ����� �����ϴ� �޸� ����
#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480

#include "Timer.h"
#include "Player.h"
#include "Scene.h"

// ������ ��� ���п� ��ũ��

// ���� ����ϴ� �⺻ ������
#define DRAW_SCENE_COLOR				'S'
// �ؽ��ĸ� ������ ������
#define DRAW_SCENE_TEXTURE				'T'
// ���� ����� ���Ե� ������
#define DRAW_SCENE_LIGHTING				'L'
// ��� ���� ������
#define DRAW_SCENE_NORMAL				'N'
// Z ���۰� ������
// Z ���۰�: GPU�� �ȼ��� ���̸� �����ϴ� ���� �޸�
#define DRAW_SCENE_Z_DEPTH				'Z'
// ���̰� ������
// ���̰�: ī�޶� �������� ��ü������ �Ÿ���
#define DRAW_SCENE_DEPTH				'D'

// �����ӿ�ũ�� ǥ���ϴ� Ŭ����
class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	// Init
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	// �Ҹ��ڿ��� �� ��
	void OnDestroy();

	// SwapChain, Device, CommandQueue/List ����
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	// RTV, DSV DescriptorHeap ����
	void CreateRtvAndDsvDescriptorHeaps();

	// RTV, DSV ����
	void CreateSwapChainRenderTargetViews();
	void CreateDepthStencilView();

	// 
	void ChangeSwapChainState();

	// Object ����, ����
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
	// ���α׷� �ν��Ͻ� �ڵ�
	HINSTANCE						m_hInstance;
	// ������ â �ڵ�
	HWND							m_hWnd; 

	// ������ â ������
	int								m_nWndClientWidth;
	int								m_nWndClientHeight;
        
	// SwapChain, Device�� ����ϱ� ���� ��ü
	IDXGIFactory4					*m_pdxgiFactory = NULL;
	// �������� ���� BackBuffer, FrontBuffer�� ��ü���ִ� ��ü
	IDXGISwapChain3					*m_pdxgiSwapChain = NULL;
	// GPU�� ���� ����ϴ� ��ü
	ID3D12Device					*m_pd3dDevice = NULL;

	// ��� ���� ���� ����
	// MSAA ��� ����
	bool							m_bMsaa4xEnable = false;
	// MSAA ǰ�� ����
	UINT							m_nMsaa4xQualityLevels = 0;

	// SwapChine BackBuffer ��
	static const UINT				m_nSwapChainBuffers = 2;
	// ���� ��� ���� SwapChain�� BackBuffer ��ġ�� ����ϴ� ����
	UINT							m_nSwapChainBufferIndex = 0;

	// SwapChin�� ���� BackBuffer�� ������ �迭
	ID3D12Resource					*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	
	// GPU�� ����� RTV�� ��Ƶ� DescriptorHeap 
	// RTV: GPU�� SwapChain BackBuffer�� �����ϱ� ���� �ʿ��� View
	ID3D12DescriptorHeap			*m_pd3dRtvDescriptorHeap = NULL;
	// SwapChain���� ������ DescriptorHeap�� �ִ� RTV �ڵ��� ����
	D3D12_CPU_DESCRIPTOR_HANDLE		m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBuffers];

	// DSV�� ����� Buffer
	ID3D12Resource					*m_pd3dDepthStencilBuffer = NULL;
	// GPU�� ����� DSV�� ��Ƶ� DescriptorHeap
	// DSV: GPU�� DepthStencilBuffer�� �����ϱ� ���� �ʿ��� View
	ID3D12DescriptorHeap			*m_pd3dDsvDescriptorHeap = NULL;
	// DescriptorHeap�� �ִ� DSV �ڵ�
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dDsvDescriptorCPUHandle;

	// CommandList�� ����� ����� ������ �����ϴ� ��ü
	ID3D12CommandAllocator			*m_pd3dCommandAllocator = NULL;
	// CommandList ��ü�� �׾Ƶδ� ��ü
	// GPU�� CommandQueue���� CommandList�� ���� 
	ID3D12CommandQueue				*m_pd3dCommandQueue = NULL;
	// CommandAllocator�� �Ҵ��� ������ ����� ����ϴ� ��ü
	ID3D12GraphicsCommandList		*m_pd3dCommandList = NULL;

	// CPU, GPU ����ȭ�� ��ü
	ID3D12Fence						*m_pd3dFence = NULL;
	// SwapChain BackBuffer���� ����� Fence��
	UINT64							m_nFenceValues[m_nSwapChainBuffers];
	// CPU�� GPU�� �۾��� ��ٸ� �� ����ϴ� �̺�Ʈ �ڵ�
	HANDLE							m_hFenceEvent;

	CGameTimer						m_GameTimer;

	CScene							*m_pScene = NULL;
	CPlayer							*m_pPlayer = NULL;
	CCamera							*m_pCamera = NULL;

	// ��ó�� Shader
	CPostProcessingShader			*m_pPostProcessingShader = NULL;

	// ������ ���
	int	m_nDrawOption = DRAW_SCENE_COLOR;

	// ���콺 ���� ��ǥ
	POINT m_ptOldCursorPos;

	// FrameRate�� ���ڿ��� �����ϱ� ���� ����
	_TCHAR	m_pszFrameRate[50];
};