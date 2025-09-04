// src/Universal/GraphicsDetection.h
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <functional>

enum class RenderingBackend {
    UNKNOWN,
    DIRECTX9,
    DIRECTX11,
    DIRECTX12,
    OPENGL,
    VULKAN
};

struct GraphicsInfo {
    RenderingBackend backend;
    std::string adapterName;
    std::string driverVersion;
    HWND renderWindow;
    void* devicePtr; // Device pointer (varies by API)
    void* contextPtr; // Context pointer (varies by API)
    uint32_t backBufferWidth;
    uint32_t backBufferHeight;
    bool isFullscreen;
    float refreshRate;
};

// Hook information for different APIs
struct GraphicsHooks {
    // DirectX 9
    void* d3d9Present = nullptr;
    void* d3d9Reset = nullptr;
    void* d3d9EndScene = nullptr;
    
    // DirectX 11
    void* d3d11Present = nullptr;
    void* d3d11ResizeBuffers = nullptr;
    void* d3d11DrawIndexed = nullptr;
    
    // DirectX 12
    void* d3d12Present = nullptr;
    void* d3d12ExecuteCommandLists = nullptr;
    
    // OpenGL
    void* wglSwapBuffers = nullptr;
    void* glDrawElements = nullptr;
    
    // Vulkan
    void* vkQueuePresentKHR = nullptr;
    void* vkCmdDrawIndexed = nullptr;
};

// Callback types for rendering integration
using PresentCallback = std::function<void(void* device, void* backBuffer)>;
using DrawCallback = std::function<void(void* device, void* commandList)>;
using ResizeCallback = std::function<void(uint32_t width, uint32_t height)>;

class UniversalGraphicsDetection {
public:
    static UniversalGraphicsDetection& GetInstance();
    
    // Core detection methods
    bool Initialize(DWORD processId);
    void Cleanup();
    
    RenderingBackend DetectGraphicsAPI();
    GraphicsInfo GetGraphicsInfo();
    
    // Hook management
    bool InstallHooks();
    void RemoveHooks();
    bool IsHooked() const { return m_hooksInstalled; }
    
    // Callback registration
    void RegisterPresentCallback(PresentCallback callback);
    void RegisterDrawCallback(DrawCallback callback);
    void RegisterResizeCallback(ResizeCallback callback);
    
    // ImGui integration
    bool InitializeImGui();
    void ShutdownImGui();
    void BeginFrame();
    void EndFrame();
    void RenderFrame();
    
    // Overlay management
    bool CreateOverlay();
    void DestroyOverlay();
    void UpdateOverlayPosition();
    void SetOverlayVisible(bool visible);
    
    // Utility methods
    HWND GetRenderWindow();
    void* GetDevice();
    void* GetContext();
    std::pair<uint32_t, uint32_t> GetBackBufferSize();
    
private:
    UniversalGraphicsDetection() = default;
    
    DWORD m_processId = 0;
    RenderingBackend m_detectedBackend = RenderingBackend::UNKNOWN;
    GraphicsInfo m_graphicsInfo = {};
    GraphicsHooks m_hooks = {};
    
    bool m_initialized = false;
    bool m_hooksInstalled = false;
    bool m_imguiInitialized = false;
    bool m_overlayCreated = false;
    
    // Callbacks
    std::vector<PresentCallback> m_presentCallbacks;
    std::vector<DrawCallback> m_drawCallbacks;
    std::vector<ResizeCallback> m_resizeCallbacks;
    
    // DirectX 9 detection and setup
    bool DetectDirectX9();
    bool SetupDirectX9Hooks();
    void CleanupDirectX9();
    bool InitializeImGuiDX9();
    
    // DirectX 11 detection and setup
    bool DetectDirectX11();
    bool SetupDirectX11Hooks();
    void CleanupDirectX11();
    bool InitializeImGuiDX11();
    
    // DirectX 12 detection and setup
    bool DetectDirectX12();
    bool SetupDirectX12Hooks();
    void CleanupDirectX12();
    bool InitializeImGuiDX12();
    
    // OpenGL detection and setup
    bool DetectOpenGL();
    bool SetupOpenGLHooks();
    void CleanupOpenGL();
    bool InitializeImGuiOpenGL();
    
    // Vulkan detection and setup
    bool DetectVulkan();
    bool SetupVulkanHooks();
    void CleanupVulkan();
    bool InitializeImGuiVulkan();
    
    // Hook functions (static for C-style callbacks)
    static HRESULT STDMETHODCALLTYPE PresentHook_DX9(void* device, const RECT* sourceRect, const RECT* destRect, HWND destWindow, const RGNDATA* dirtyRegion);
    static HRESULT STDMETHODCALLTYPE PresentHook_DX11(void* swapChain, UINT syncInterval, UINT flags);
    static HRESULT STDMETHODCALLTYPE PresentHook_DX12(void* swapChain, UINT syncInterval, UINT flags);
    static BOOL WINAPI SwapBuffersHook(HDC hdc);
    static VkResult VKAPI_CALL QueuePresentHook(VkQueue queue, const VkPresentInfoKHR* presentInfo);
    
    // Utility methods
    std::vector<std::wstring> GetLoadedModules();
    void* GetModuleFunction(const std::wstring& moduleName, const std::string& functionName);
    bool IsProcessX64();
    void LogDetectionInfo();
    
    // Memory management
    void* AllocateExecutableMemory(size_t size);
    void FreeExecutableMemory(void* memory);
    
    // Window management
    HWND FindRenderWindow();
    bool IsRenderWindow(HWND hwnd);
    void GetWindowInfo(HWND hwnd);
};