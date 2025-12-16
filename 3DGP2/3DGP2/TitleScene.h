#pragma once
#include "Scene.h"

class TitleScene : public Scene
{
public:
	TitleScene(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
	virtual ~TitleScene();

public:
	virtual void ProcessInput(HWND hwnd, ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList);
};