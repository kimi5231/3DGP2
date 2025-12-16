#include "pch.h"
#include "GameScene.h"
#include "GameObject.h"
#include "Shader.h"
#include "Mesh.h"

GameScene::GameScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_shader = std::make_shared<Shader>();
	_shader->CreateShaderTessellation(device);
	_uploadBuffers.resize(1);
	_textures.push_back(CreateTextureResourceFromDDSFile(device.Get(), commandList.Get(), (wchar_t*)(L"Resource\\Stone01.dds"), _uploadBuffers[0].GetAddressOf(), D3D12_RESOURCE_STATE_GENERIC_READ));
	_shader->CreateShaderResourceView(_textures, device, commandList);

	{
		Mesh* mesh = new Mesh();
		Vertex vertices[] =
		{
			Vertex(XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(Colors::Red), XMFLOAT2(0.0f, 0.0f)),
			Vertex(XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(Colors::Green), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f)),
		};
		mesh->SetVertexCount(3);
		mesh->SetTriangleTessellation(device, commandList, vertices);

		GameObjectShared object = std::make_shared<GameObject>();
		object->SetID(0);
		object->SetMesh(mesh);
		_objects.push_back(object);
	}

	_isWireframe = true;
}

GameScene::~GameScene()
{
}

void GameScene::ProcessInput(HWND hwnd, ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
{
	if ((GetAsyncKeyState(VK_LBUTTON) & 0x0001))
	{
		_shader = nullptr;
		_shader = std::make_shared<Shader>();
		if (_isWireframe)
			_shader->CreateShader(device);
		else
			_shader->CreateShaderTessellation(device);
		_shader->CreateShaderResourceView(_textures, device, commandList);
		
		Mesh* mesh = new Mesh();
		Vertex vertices[] =
		{
			Vertex(XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(Colors::Red), XMFLOAT2(0.0f, 0.0f)),
			Vertex(XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(Colors::Green), XMFLOAT2(1.0f, 0.0f)),
			Vertex(XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f)),
		};
		mesh->SetVertexCount(3);
		if (_isWireframe)
		{
			_isWireframe = false;
			mesh->SetTriangle(device, commandList, vertices);
		}
		else
		{
			_isWireframe = true;
			mesh->SetTriangleTessellation(device, commandList, vertices);
		}

		_objects[0]->SetMesh(mesh);
	}
}