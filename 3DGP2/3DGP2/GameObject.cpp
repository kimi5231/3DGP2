#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

void GameObject::Update(ShaderShared shader)
{

}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList> commandList)
{
	if (_mesh) 
		_mesh->Render(commandList);
}

void GameObject::ReleaseUploadBuffer()
{
	if (_mesh)
		_mesh->ReleaseUploadBuffer();
}