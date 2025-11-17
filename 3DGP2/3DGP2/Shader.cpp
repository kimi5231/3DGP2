#include "pch.h"
#include "Shader.h"

Shader::Shader()
{
}

Shader::~Shader()
{
}

void Shader::Update()
{
}

void Shader::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->SetGraphicsRootSignature(_graphicsRootSignature.Get());
	commandList->SetDescriptorHeaps(1, _srvDescriptorHeap.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(0, _srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetPipelineState(_pipelineState.Get());
}

void Shader::CreateGraphicsRootSignature(ComPtr<ID3D12Device> device)
{
	D3D12_DESCRIPTOR_RANGE descriptorRanges;
	descriptorRanges.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges.NumDescriptors = 1;
	descriptorRanges.BaseShaderRegister = 0;
	descriptorRanges.RegisterSpace = 0;
	descriptorRanges.OffsetInDescriptorsFromTableStart = 0;

	D3D12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	::ZeroMemory(&samplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	::ZeroMemory(&rootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	rootSignatureDesc.NumParameters = _countof(rootParameters);
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 1;
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.Flags = d3dRootSignatureFlags;

	ComPtr<ID3DBlob> signatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	// RootSignature 생성에 필요한 정보를 GPU가 이해할 수 있도록 직렬화
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	// RootSignature 생성
	device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&_graphicsRootSignature);
}

void Shader::CreateShader(ComPtr<ID3D12Device> device)
{
	// RootSinature 생성
	CreateGraphicsRootSignature(device);

	// Pipeline State
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	::ZeroMemory(&pipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pipelineStateDesc.pRootSignature = _graphicsRootSignature.Get();
	// VertexShader 정보 얻어오기
	pipelineStateDesc.VS = CreateVertexShader(&vertexShaderBlob);
	// PixelShader 정보 얻어오기
	pipelineStateDesc.PS = CreatePixelShader(&pixelShaderBlob);
	pipelineStateDesc.RasterizerState = CreateRasterizerState();
	pipelineStateDesc.BlendState = CreateBlendState();
	pipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	pipelineStateDesc.InputLayout = CreateInputLayout();
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	// Pipeline State 생성
	device->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)_pipelineState.GetAddressOf());

	if (pipelineStateDesc.InputLayout.pInputElementDescs)
		delete[] pipelineStateDesc.InputLayout.pInputElementDescs;
}

void Shader::CreateCbvSrvDescriptorHeaps(ComPtr<ID3D12Device> device)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&_srvDescriptorHeap);
}

void Shader::CreateShaderResourceView(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	CreateCbvSrvDescriptorHeaps(device);

	_texture = CreateTextureResourceFromDDSFile(device.Get(), commandList.Get(), (wchar_t*)(L"Stone01.dds"), _uploadBuffer.GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = _texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = _texture->GetDesc().MipLevels;
	device->CreateShaderResourceView(_texture.Get(), &srvDesc, _srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

D3D12_SHADER_BYTECODE Shader::CreateVertexShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"Shader.hlsl", "VSMain", "vs_5_1", shaderBlob);
}

D3D12_SHADER_BYTECODE Shader::CreatePixelShader(ID3DBlob** shaderBlob)
{
	return CompileShaderFromFile(L"Shader.hlsl", "PSMain", "ps_5_1", shaderBlob);
}

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}

D3D12_BLEND_DESC Shader::CreateBlendState()
{
	D3D12_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

D3D12_DEPTH_STENCIL_DESC Shader::CreateDepthStencilState()
{
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}

D3D12_INPUT_LAYOUT_DESC Shader::CreateInputLayout()
{
	const UINT inputElementDescNum = 3;
	D3D12_INPUT_ELEMENT_DESC* inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[inputElementDescNum];

	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = inputElementDescNum;

	return inputLayoutDesc;
}

D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(const wchar_t* fileName, LPCSTR shaderName, LPCSTR shaderProfile, ID3DBlob** shaderBlob)
{
	UINT compileFlags{};
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	D3DCompileFromFile(fileName, nullptr, nullptr, shaderName, shaderProfile, compileFlags, 0, shaderBlob, nullptr);

	D3D12_SHADER_BYTECODE shaderBytecode{};
	shaderBytecode.BytecodeLength = (*shaderBlob)->GetBufferSize();
	shaderBytecode.pShaderBytecode = (*shaderBlob)->GetBufferPointer();

	return shaderBytecode;
}

void Shader::ReleaseUploadBuffers()
{
	
}