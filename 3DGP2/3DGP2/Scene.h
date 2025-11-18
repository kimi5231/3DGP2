#pragma once
class GameObject;
class Shader;

class Scene
{
public:
	Scene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	virtual ~Scene();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

public:
	void ReleaseUploadBuffers();

protected:
	ShaderShared _shader;
	std::vector<GameObjectShared> _objects;
};