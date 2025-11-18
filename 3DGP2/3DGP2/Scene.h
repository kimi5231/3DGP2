#pragma once
class GameObject;
class Shader;

class Scene
{
public:
	Scene();
	virtual ~Scene();

public:
	void Update();
	void Render(ComPtr<ID3D12GraphicsCommandList> commandList);

public:
	void ReleaseUploadBuffers();

protected:
	ShaderShared _shader;
	std::vector<GameObjectShared> _objects;
	std::vector<ComPtr<ID3D12Resource>> _textures;
	std::vector<ComPtr<ID3D12Resource>> _uploadBuffers;
};