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

inline ID3D12Resource* CreateBufferResource(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, void* data, UINT bytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceStates, ID3D12Resource** uploadBuffer)
{
	ID3D12Resource* buffer{};

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = heapType;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = bytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_RESOURCE_STATES resourceInitialState = D3D12_RESOURCE_STATE_COPY_DEST;
	if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
		resourceInitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else if (heapType == D3D12_HEAP_TYPE_READBACK) {
		resourceInitialState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		resourceInitialState,
		nullptr,
		__uuidof(ID3D12Resource),
		(void**)&buffer
	);

	if (data) {
		switch (heapType) {
		case D3D12_HEAP_TYPE_DEFAULT:
		{
			// 업로드 버퍼 생성
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				__uuidof(ID3D12Resource),
				(void**)uploadBuffer
			);

			// 업로드 버퍼를 매핑하여 초기화 데이터를 버퍼에 복사
			D3D12_RANGE readRange{ 0, 0 };
			UINT8* bufferDataBegin{};
			(*uploadBuffer)->Map(0, &readRange, (void**)(&bufferDataBegin));
			memcpy(bufferDataBegin, data, bytes);
			(*uploadBuffer)->Unmap(0, nullptr);

			// 업로드 버퍼 내용을 디폴트 버퍼에 복사
			commandList->CopyResource(buffer, *uploadBuffer);

			D3D12_RESOURCE_BARRIER resourceBarrier{};
			resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			resourceBarrier.Transition.pResource = buffer;
			resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			resourceBarrier.Transition.StateAfter = resourceStates;
			resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			commandList->ResourceBarrier(1, &resourceBarrier);
		}
		break;
		case D3D12_HEAP_TYPE_UPLOAD:
		{
			// 업로드 버퍼를 매핑하여 초기화 데이터를 버퍼에 복사
			D3D12_RANGE readRange{ 0, 0 };
			UINT8* bufferDataBegin{};
			buffer->Map(0, &readRange, (void**)(&bufferDataBegin));
			memcpy(bufferDataBegin, data, bytes);
			buffer->Unmap(0, nullptr);
		}
		break;
		default:
			break;
		}
	}

	return buffer;
}

inline ID3D12Resource* CreateTextureResourceFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, wchar_t* pszFileName, ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates)
{
	ID3D12Resource* pd3dTexture = NULL;
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> vSubresources;
	DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool bIsCubeMap = false;

	HRESULT hResult = DirectX::LoadDDSTextureFromFileEx(pd3dDevice, pszFileName, 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &pd3dTexture, ddsData, vSubresources, &ddsAlphaMode, &bIsCubeMap);

	D3D12_HEAP_PROPERTIES d3dHeapPropertiesDesc;
	::ZeroMemory(&d3dHeapPropertiesDesc, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	d3dHeapPropertiesDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapPropertiesDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapPropertiesDesc.CreationNodeMask = 1;
	d3dHeapPropertiesDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pd3dTexture->GetDesc();
	UINT nSubResources = (UINT)vSubresources.size();
	UINT64 nBytes = GetRequiredIntermediateSize(pd3dTexture, 0, nSubResources);
	//	UINT nSubResources = d3dTextureResourceDesc.DepthOrArraySize * d3dTextureResourceDesc.MipLevels;
	//	UINT64 nBytes = 0;
	//	pd3dDevice->GetCopyableFootprints(&d3dTextureResourceDesc, 0, nSubResources, 0, NULL, NULL, NULL, &nBytes);

	D3D12_RESOURCE_DESC d3dBufferResourceDesc;
	::ZeroMemory(&d3dBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	d3dBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; //Upload Heap에는 텍스쳐를 생성할 수 없음
	d3dBufferResourceDesc.Alignment = 0;
	d3dBufferResourceDesc.Width = nBytes;
	d3dBufferResourceDesc.Height = 1;
	d3dBufferResourceDesc.DepthOrArraySize = 1;
	d3dBufferResourceDesc.MipLevels = 1;
	d3dBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	d3dBufferResourceDesc.SampleDesc.Count = 1;
	d3dBufferResourceDesc.SampleDesc.Quality = 0;
	d3dBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	d3dBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void**)ppd3dUploadBuffer);

	//UINT nSubResources = (UINT)vSubresources.size();
	//D3D12_SUBRESOURCE_DATA *pd3dSubResourceData = new D3D12_SUBRESOURCE_DATA[nSubResources];
	//for (UINT i = 0; i < nSubResources; i++)pd3dSubResourceData[i] = vSubresources.at(i);

	//	std::vector<D3D12_SUBRESOURCE_DATA>::pointer ptr = &vSubresources[0];
	UINT64 nBytesUpdated = ::UpdateSubresources(pd3dCommandList, pd3dTexture, *ppd3dUploadBuffer, 0, 0, nSubResources, &vSubresources[0]);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = pd3dTexture;
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	//	delete[] pd3dSubResourceData;

	return(pd3dTexture);
}