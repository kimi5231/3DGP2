#include "pch.h"
#include "GameScene.h"

GameScene::GameScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList)
	: Scene(device, commandList)
{
}

GameScene::~GameScene()
{
}