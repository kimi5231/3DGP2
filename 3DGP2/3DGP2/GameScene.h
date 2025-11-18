#pragma once
#include "Scene.h"

class GameScene : public Scene
{
public:
	GameScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	virtual ~GameScene();
};