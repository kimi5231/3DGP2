#include "pch.h"
#include "Mesh.h"

CVertex::CVertex(const CVertex& rhs) : position{ rhs.position }
{

}

CVertex& CVertex::operator=(const CVertex& rhs)
{
	if (this == &rhs) return *this;
	position = rhs.position;

	return *this;
}

CVertex::CVertex(CVertex&& other) : position{ std::move(other.position) }
{
}

CVertex& CVertex::operator=(CVertex&& other)
{
	if (this == &other) return *this;
	position = std::move(other.position);

	return *this;
}

Mesh::Mesh(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
}

void Mesh::ReleaseUploadBuffer()
{
	if (vertex_upload_buffer) 
		vertex_upload_buffer.Reset();
	vertex_upload_buffer = nullptr;
}

void Mesh::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// 프리미티브 유형 설정
	commandList->IASetPrimitiveTopology(primitive_topology);
	// 정점 버퍼 뷰 설정
	commandList->IASetVertexBuffers(slot_num, 1, &vertex_buffer_view);
	// 렌더링(입력 조립기 작동)
	commandList->DrawInstanced(vertex_num, 1, offset, 0);
}

CTriangleMesh::CTriangleMesh(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList) :
	Mesh(device, commandList)
{
	vertex_num = 3;
	stride = sizeof(CDiffusedVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CDiffusedVertex vertices[] = {
		CDiffusedVertex(XMFLOAT3(0.0f, 0.5, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
		CDiffusedVertex(XMFLOAT3(0.5f, -0.5, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f)),
		CDiffusedVertex(XMFLOAT3(-0.5f, -0.5, 0.0f), XMFLOAT4(Colors::Blue))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}
