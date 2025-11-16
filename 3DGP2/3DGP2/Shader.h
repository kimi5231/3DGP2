#pragma once
class Shader
{
public:
	Shader();
	~Shader();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

public:
	void CreateGraphicsRootSignature(ComPtr<ID3D12Device>);

	virtual void CreateShader(ComPtr<ID3D12Device> device);

	// piplineState 설정을 위한 함수
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** shaderBlob);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** shaderBlob);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();

	// Shader Compile
	D3D12_SHADER_BYTECODE CompileShaderFromFile(const wchar_t* fileName, LPCSTR shaderName, LPCSTR shaderProfile, ID3DBlob** shaderBlob);

	virtual void ReleaseUploadBuffers();

public:
	ID3D12RootSignature* GetGraphicsRootSignature() { return _graphicsRootSignature.Get(); }
	
private:
	ComPtr<ID3D12RootSignature> _graphicsRootSignature{};
	ComPtr<ID3D12PipelineState> _pipelineState{};
};