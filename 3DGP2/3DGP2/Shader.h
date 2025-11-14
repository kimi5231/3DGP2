#pragma once
class Shader
{
public:
	Shader();
	~Shader();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

	// 입력 조립기에게 정점 버퍼를 알려주기 위한 구조체 반환
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	// 래스터라이저 상태를 설정하기 위한 구조체 반환
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	// 블렌드 상태를 설정하기 위한 구조체 반환
	virtual D3D12_BLEND_DESC CreateBlendState();
	// 깊이-스텐실 검사를 위한 상태를 설정하기 위한 구조체 반환
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	// 셰이더 코드 컴파일
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**);
	D3D12_SHADER_BYTECODE CompileShaderFromFile(const wchar_t*, LPCSTR, LPCSTR, ID3DBlob**);

	//그래픽스 파이프라인 상태 객체 생성
	virtual void CreateShader(ComPtr<ID3D12Device>);
	//virtual void CreateShaderVariables(ID3D12Device*, ID3D12GraphicsCommandList*);
	//virtual void UpdateShaderVariables(ID3D12GraphicsCommandList*);
	//virtual void ReleaseShaderVariables();
	virtual void ReleaseUploadBuffers();

	// 루트 시그니처
	ID3D12RootSignature* GetGraphicsRootSignature() { return graphics_root_signature.Get(); }
	ID3D12RootSignature* CreateGraphicsRootSignature(ComPtr<ID3D12Device>);

protected:
	ComPtr<ID3D12RootSignature> graphics_root_signature{};
	std::deque<ComPtr<ID3D12PipelineState>> pipeline_states{};
};