#pragma once
class GameObject;
class Shader;

class Scene
{
public:
	Scene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	~Scene();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

	void ReleaseUploadBuffers();

private:
	std::vector<ShaderShared> _shaders;
	std::vector<GameObjectShared> _objects;
};