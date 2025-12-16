#pragma once
#include "Scene.h"

class GameScene : public Scene
{
public:
	GameScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	virtual ~GameScene();

public:
	virtual void ProcessInput(HWND hwnd, ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	bool _isWireframe;
};