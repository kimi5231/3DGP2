#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Shader.h"
#include "Mesh.h"

Scene::Scene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_shader = std::make_shared<Shader>();
	_shader->CreateShader(device);
	_shader->CreateShaderResourceView(device, commandList);
}

Scene::~Scene()
{
}

void Scene::Update()
{

}

void Scene::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (GameObjectShared object : _objects)
	{
		_shader->Render(object->GetID(), commandList);
		object->Render(commandList);
	}
}

void Scene::ReleaseUploadBuffers()
{
	for (GameObjectShared object : _objects)
		object->ReleaseUploadBuffer();

	_shader->ReleaseUploadBuffers();
}