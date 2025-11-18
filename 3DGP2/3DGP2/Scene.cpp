#include "pch.h"
#include "Scene.h"
#include "GameObject.h"
#include "Shader.h"

Scene::Scene()
{
	
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
		if(_shader)
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