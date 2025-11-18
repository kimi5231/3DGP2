#pragma once
class Mesh;

class GameObject
{
public:
	GameObject();
	~GameObject();

public:
	void Update(ShaderShared shader);
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

public:
	void ReleaseUploadBuffer();

public:
	void SetID(UINT id) { _id = id; }
	UINT GetID() { return _id; }
	void SetMesh(Mesh* mesh) { _mesh = mesh; };

private:
	UINT _id;
	Mesh* _mesh;
};