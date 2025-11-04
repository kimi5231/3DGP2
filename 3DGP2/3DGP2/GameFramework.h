#pragma once
class GameFramework
{
public:
	GameFramework();
	~GameFramework();

public:
	void Update();
	void Render();

private:
	void CreateDevice();
	void CreateSwapChine();

};