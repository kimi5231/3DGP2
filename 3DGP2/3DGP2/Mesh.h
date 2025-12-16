struct Vertex
{
public:
	XMFLOAT3 pos{};
	XMFLOAT4 color{};
	XMFLOAT2 uv{};

public:
	Vertex& operator=(const Vertex& other)
	{
		pos = other.pos;
		color = other.color;
		uv = other.uv;
		return *this;
	}
};

class Mesh
{
public:
	Mesh();
	~Mesh();

public:
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>);

public:
	void SetTriangle(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, Vertex* vertices);
	void SetTriangleTessellation(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, Vertex* vertices);

	void ReleaseUploadBuffer();

public:
	void SetVertexCount(UINT count) { _vertexCount = count; }

private:
	UINT _vertexCount{};
	// 정점 간의 간격
	UINT _stride{};
	D3D12_PRIMITIVE_TOPOLOGY _primitiveTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	
	ComPtr<ID3D12Resource> _vertexBuffer{};
	ComPtr<ID3D12Resource> _uploadBuffer{};
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView{};

	// 리소스를 연결할 슬롯 번호
	UINT _slotNum{};
	// 정점 버퍼 시작 위치
	UINT _offset{};
};