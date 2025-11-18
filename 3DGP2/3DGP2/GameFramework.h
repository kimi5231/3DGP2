#pragma once
class Scene;

class GameFramework
{
public:
	GameFramework(HWND hwnd);
	~GameFramework();

public:
	void Update();
	void Render();

private:
	// Device 생성, 이 함수 내에서 Factory, Aapter도 생성
	void CreateDevice();
	void CreateCommandQueueAndList();
	void CreateSwapChain();

	void CreateRtvDsvDescriptorHeaps();
	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void WaitForGpuComplete();
	void MoveToNextFrame();

public:
	void ProcessInput();

	void ChangeScene();

private:
	Scene* _scene;

private:
	// 윈도우 창 핸들
	HWND _hwnd{};
	
	// Device, Adapter, SwapChain를 사용하기 위한 객체
	ComPtr<IDXGIFactory4> _factory{};
	// GPU와 직접 통신하는 객체 
	ComPtr<ID3D12Device> _device{};

	// 렌더링을 위한 BackBuffer, FrontBuffer를 교체해주는 객체
	ComPtr<IDXGISwapChain3> _swapChain{};
	// SwapChine BackBuffer 수
	static const UINT _swapChainBufferCount{2};
	// 현재 사용 중인 SwapChain의 BackBuffer 위치를 기록하는 변수
	UINT _swapChainBufferIndex{};
	// SwapChin의 내부 BackBuffer를 참조할 배열
	ComPtr<ID3D12Resource> _swapChainBackBuffers[_swapChainBufferCount]{};

	// CommandList가 명령을 기록할 공간을 관리하는 객체
	ComPtr<ID3D12CommandAllocator> _commandAllocator{};
	// CommandList 자체를 쌓아두는 객체
	// GPU가 CommandQueue에서 CommandList를 빼감
	ComPtr<ID3D12CommandQueue> _commandQueue{};
	// CommandAllocator가 할당한 공간에 명령을 기록하는 객체
	ComPtr<ID3D12GraphicsCommandList> _commandList{};

	// GPU가 사용할 RTV를 담아둘 DescriptorHeap 
	// RTV: GPU가 SwapChain BackBuffer에 접근하기 위해 필요한 View
	ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap{};
	// RTV 건너 뛸 값
	UINT _rtvDescriptorIncrementSize{};

	// GPU가 사용할 DSV를 담아둘 DescriptorHeap
	// DSV: GPU가 DepthStencilBuffer에 접근하기 위해 필요한 View
	ComPtr<ID3D12DescriptorHeap> _dsvDescriptorHeap{};
	// DSV와 연결될 Buffer
	ComPtr<ID3D12Resource> _depthStencilBuffer{};

	// 계단 현상 관련 변수
	// MSAA 사용 여부
	bool _msaa4xEnable{};
	// MSAA 품질 레벨의 수
	UINT _msaa4xQualityLevel{};

	// CPU, GPU 동기화용 객체
	ComPtr<ID3D12Fence> _fence{};
	// SwapChain BackBuffer마다의 목표 Fence값
	UINT64 _fenceValues[_swapChainBufferCount]{};
	// CPU가 GPU의 작업을 기다릴 때 사용하는 이벤트 핸들
	HANDLE _fenceEvent{};



	// 뷰포트와 씨저 사각형
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissor_rect{};
};