#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Shader.h"
#include "Mesh.h"


Scene::Scene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ShaderShared shader = std::make_shared<Shader>();
	shader->CreateShader(device);
	shader->CreateShaderResourceView(device, commandList);
	_shaders.push_back(shader);

	Mesh* mesh = new Mesh();
	Vertex vertices[] = 
	{
		// 첫번째 삼각형
		Vertex(XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)),
		Vertex(XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
		Vertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f)),
		// 두번째 삼각형
		Vertex(XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
		Vertex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(1.0f, 1.0f)),
		Vertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f))
	};
	mesh->SetVertexCount(6);
	mesh->SetTriangle(device, commandList, vertices);

	GameObjectShared object = std::make_shared<GameObject>();
	object->SetMesh(mesh);
	_objects.push_back(object);
}

Scene::~Scene()
{
}

void Scene::Update()
{
	for (GameObjectShared object : _objects)
		object->Update();
}

void Scene::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (ShaderShared shader : _shaders)
		shader->Render(commandList);

	for (GameObjectShared object : _objects)
		object->Render(commandList);
}

void Scene::ReleaseUploadBuffers()
{
	for (GameObjectShared object : _objects)
		object->ReleaseUploadBuffer();

	for (ShaderShared shader : _shaders)
		shader->ReleaseUploadBuffers();
}