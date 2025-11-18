#pragma once

#include <iostream>
#include <vector>
#include <deque>

#include <windows.h>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <d3dcompiler.h>

#include "DDSTextureLoader12.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

#define ScreenWidth 800
#define ScreenHeight 600

using namespace DirectX;
using Microsoft::WRL::ComPtr;

using ShaderShared = std::shared_ptr<class Shader>;
using GameObjectShared = std::shared_ptr<class GameObject>;

ID3D12Resource* CreateBufferResource(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, void* data, UINT bytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceStates, ID3D12Resource** uploadBuffer);

ID3D12Resource* CreateTextureResourceFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, wchar_t* pszFileName, ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates);