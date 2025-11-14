#pragma once
class Mesh;

class GameObject
{
public:
	GameObject();
	~GameObject();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

	void ReleaseUploadBuffer();

public:
	void SetMesh(Mesh* mesh);

private:
	Mesh* _mesh;
};