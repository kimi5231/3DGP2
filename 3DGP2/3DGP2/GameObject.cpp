#include "pch.h"
#include "GameObject.h"
#include "Mesh.h"

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

void GameObject::Update()
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

void GameObject::SetMesh(Mesh* mesh)
{
	_mesh = mesh;
}
