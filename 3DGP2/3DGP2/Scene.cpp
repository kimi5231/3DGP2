#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Shader.h"
#include "Mesh.h"

Scene::Scene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ShaderShared shader = std::make_shared<Shader>();
	shader->CreateShader(device);
	_shaders.push_back(shader);

	GameObjectShared object = std::make_shared<GameObject>();
	CTriangleMesh* pTriangleMesh = new CTriangleMesh(device, commandList);
	object->SetMesh(pTriangleMesh);
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