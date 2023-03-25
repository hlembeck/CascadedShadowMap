#include "DescriptorHeaps.h"

void DescriptorHeaps::LoadCBV(ID3D12Device* device) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		MAXDESCRIPTORS,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvHeap));
}

void DescriptorHeaps::LoadSampler(ID3D12Device* device) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		MAXDESCRIPTORS,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_samplerHeap));
}

void DescriptorHeaps::LoadRTV(ID3D12Device* device) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		MAXDESCRIPTORS,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvHeap)));
}

void DescriptorHeaps::LoadDSV(ID3D12Device* device) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		MAXDESCRIPTORS,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dsvHeap));
}

void DescriptorHeaps::OnInit(ID3D12Device* device) {
	m_incrementSize[0] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_incrementSize[1] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_incrementSize[2] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_incrementSize[3] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	LoadCBV(device);
	LoadSampler(device);
	LoadRTV(device);
	LoadDSV(device);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeaps::GetCPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) {
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	switch (type) {
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[0]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_samplerHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[1]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[2]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[3]);
		return cpuHandle;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeaps::GetGPUHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) {
	CD3DX12_GPU_DESCRIPTOR_HANDLE cpuHandle;
	switch (type) {
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		cpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[0]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		cpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_samplerHeap->GetGPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[1]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		cpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetGPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[2]);
		return cpuHandle;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		cpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetGPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(index, m_incrementSize[3]);
		return cpuHandle;
	}
}

ID3D12DescriptorHeap* DescriptorHeaps::GetCBVHeap() { return m_cbvHeap.Get(); }

HandlePair DescriptorHeaps::BatchHandles(D3D12_DESCRIPTOR_HEAP_TYPE type) {
	//printf("%d %d\n", type, m_usedSize[type]);
	return { GetCPUHandle(type,m_usedSize[type]),GetGPUHandle(type,m_usedSize[type]++) };
}