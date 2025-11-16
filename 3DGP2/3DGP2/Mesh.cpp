#include "pch.h"
#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// 프리미티브 유형 설정
	commandList->IASetPrimitiveTopology(_primitiveTopology);
	// GPU에게 정점 정보 넘기기
	commandList->IASetVertexBuffers(_slotNum, 1, &_vertexBufferView);
	// 렌더링
	commandList->DrawInstanced(_vertexCount, 1, _offset, 0);
}

void Mesh::SetTriangle(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, Vertex* vertices)
{
	_stride = sizeof(Vertex);
	_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점 버퍼 생성
	_vertexBuffer = CreateBufferResource(device, commandList, vertices, _stride * _vertexCount, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, _uploadBuffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = _stride;
	_vertexBufferView.SizeInBytes = _stride * _vertexCount;

	_slotNum = 0;
	_offset = 0;
}

void Mesh::ReleaseUploadBuffer()
{
	if (_uploadBuffer) 
		_uploadBuffer.Reset();
	_uploadBuffer = nullptr;
}