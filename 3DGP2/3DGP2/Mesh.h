class CVertex {
public:
	CVertex() : position{ XMFLOAT3(0.0f, 0.0f, 0.0f) } {}
	CVertex(float x, float y, float z) : position{ XMFLOAT3(x, y, z) } {}
	CVertex(XMFLOAT3 Position) : position{ Position } {}

	CVertex(const CVertex&);
	CVertex& operator=(const CVertex&);
	CVertex(CVertex&&);
	CVertex& operator=(CVertex&&);
protected:
	XMFLOAT3 position{};
};

class CDiffusedVertex : public CVertex
{
public:
	CDiffusedVertex() : CVertex(), diffuse{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) } {}
	CDiffusedVertex(XMFLOAT3 Position, XMFLOAT4 Diffuse) : CVertex(Position), diffuse(Diffuse) {}
	CDiffusedVertex(float x, float y, float z, XMFLOAT4 Diffuse) : CVertex(x, y, z), diffuse{ Diffuse } {}
protected:
	// 정점의 색상
	XMFLOAT4 diffuse;
};

class Mesh
{
public:
	Mesh(ComPtr<ID3D12Device>, ComPtr<ID3D12GraphicsCommandList>);

	void ReleaseUploadBuffer();

	virtual void Render(ComPtr<ID3D12GraphicsCommandList>);
protected:
	ComPtr<ID3D12Resource> vertex_buffer{};
	ComPtr<ID3D12Resource> vertex_upload_buffer{};

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};

	D3D12_PRIMITIVE_TOPOLOGY primitive_topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	UINT slot_num{};
	UINT vertex_num{};
	UINT stride{};
	UINT offset{};
};

class CTriangleMesh : public Mesh
{
public:
	CTriangleMesh(ComPtr<ID3D12Device>, ComPtr<ID3D12GraphicsCommandList>);
};