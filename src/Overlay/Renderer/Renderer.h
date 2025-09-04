// src/Overlay/Renderer/Renderer.h
#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <windows.h>

class Renderer {
public:
    HWND m_hWindow;
    Renderer(HWND hWnd);
    ~Renderer();
    void Render();
private:
    IDXGISwapChain3* m_pSwapChain = nullptr;
    ID3D12Device* m_pDevice = nullptr;
    ID3D12CommandQueue* m_pCommandQueue = nullptr;
    ID3D12DescriptorHeap* m_pRtvHeap = nullptr;
    // ... add more DX12 resources as needed ...
};
