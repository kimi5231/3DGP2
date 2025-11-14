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
	void CreateDevice();
	void CreateSwapChine();
	void CreateCommandQueueAndList();
	void CreateRtvAndDsvHeaps();
	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void WaitForGpuComplete();

	void MoveToNextFrame();

private:
	Scene* _scene;

private:
	HWND _hwnd;

	// directX 관련
	ComPtr<IDXGIFactory4> _factory{};
	ComPtr<IDXGISwapChain3> _swapChain{};
	ComPtr<ID3D12Device> _device{};

	// MSAA 다중 샘플링 활성화 및 다중 샘플링 레벨 설정
	bool msaa4x_enabled{};
	UINT msaa4x_quality_level{};

	static const UINT swap_chain_buffer_num{ 2 };
	UINT swap_chain_buffer_index{};

	// 렌더 타겟 버퍼, 서술자 힙 인터페이스 포인터, 렌더 타겟 서술자 원소 크기
	ComPtr<ID3D12Resource> render_target_buffers[swap_chain_buffer_num]{};
	ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap{};
	UINT rtv_increment_size{};

	// 깊이-스텐실 버퍼, 서술자 힙 인터페이스 포인터, 깊이-스텐실 서술자 원소 크기
	ComPtr<ID3D12Resource> depth_stencil_buffer{};
	ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap{};
	UINT dsv_increment_size{};

	// 명령 큐, 명형 할당자, 명령 리스트 인터페이스 포인터
	ComPtr<ID3D12CommandQueue> _commandQueue{};
	ComPtr<ID3D12CommandAllocator> _commandAllocator{};
	ComPtr<ID3D12GraphicsCommandList> _commandList{};

	// 그래픽스 파이프라인 상태 객체에 대한 인터페이스 포인터
	ComPtr<ID3D12PipelineState> pipeline_state{};

	// 펜스 인터페이스 포인터, 펜스 값, 이벤트 핸들
	ComPtr<ID3D12Fence> fence{};
	// 후면 버퍼마다 펜스값 관리
	UINT64 fence_value[swap_chain_buffer_num]{};
	HANDLE fence_event{};

	// 뷰포트와 씨저 사각형
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissor_rect{};

	Scene* now_scene{};
};