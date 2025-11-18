#include "pch.h"
#include "TitleScene.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Shader.h"

TitleScene::TitleScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
	: Scene(device, commandList)
{
	_shader = std::make_shared<Shader>();
	_shader->CreateShader(device);
	_uploadBuffers.resize(3);
	_textures.push_back(CreateTextureResourceFromDDSFile(device.Get(), commandList.Get(), (wchar_t*)(L"Resource\\Title.dds"), _uploadBuffers[0].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ));
	_textures.push_back(CreateTextureResourceFromDDSFile(device.Get(), commandList.Get(), (wchar_t*)(L"Resource\\GameStartButton.dds"), _uploadBuffers[1].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ));
	_textures.push_back(CreateTextureResourceFromDDSFile(device.Get(), commandList.Get(), (wchar_t*)(L"Resource\\GameEndButton.dds"), _uploadBuffers[2].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ));
	_shader->CreateShaderResourceView(_textures, device, commandList);

	// Game Start Button
	{
		Mesh* mesh = new Mesh();
		Vertex vertices[] =
		{
			// 첫번째 삼각형
			Vertex(XMFLOAT3(-0.9f, 0.9f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.4f, 0.9f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.9f, 0.7f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f)),
			// 두번째 삼각형
			Vertex(XMFLOAT3(-0.4f, 0.9f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.4f, 0.7f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(1.0f, 1.0f)),
			Vertex(XMFLOAT3(-0.9f, 0.7f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f))
		};
		mesh->SetVertexCount(6);
		mesh->SetTriangle(device, commandList, vertices);

		GameObjectShared object = std::make_shared<GameObject>();
		object->SetID(1);
		object->SetMesh(mesh);
		_objects.push_back(object);
	}

	// Game End Button
	{
		Mesh* mesh = new Mesh();
		Vertex vertices[] =
		{
			// 첫번째 삼각형
			Vertex(XMFLOAT3(-0.9f, 0.6f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.4f, 0.6f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.9f, 0.4f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f)),
			// 두번째 삼각형
			Vertex(XMFLOAT3(-0.4f, 0.6f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.4f, 0.4f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(1.0f, 1.0f)),
			Vertex(XMFLOAT3(-0.9f, 0.4f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f))
		};
		mesh->SetVertexCount(6);
		mesh->SetTriangle(device, commandList, vertices);

		GameObjectShared object = std::make_shared<GameObject>();
		object->SetID(2);
		object->SetMesh(mesh);
		_objects.push_back(object);
	}

	// Title
	{
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
		object->SetID(0);
		_objects.push_back(object);
	}
}

TitleScene::~TitleScene()
{
}