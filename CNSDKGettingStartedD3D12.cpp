#include <stdio.h>
#include <assert.h>
#include "framework.h"

// CNSDK includes
#include "leia/sdk/sdk.hpp"
#include "leia/sdk/interlacer.hpp"
#include "leia/sdk/debugMenu.hpp"
#include "leia/common/platform.hpp"

// CNSDKGettingStartedD3D11 includes
#include "CNSDKGettingStartedD3D12.h"
#include "CNSDKGettingStartedMath.h"

// D3D11 includes.
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

// CNSDK single library
#pragma comment(lib, "CNSDK/lib/leiaSDK-faceTrackingInApp.lib")

// D3D12 libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) if(x != nullptr) { x->Release(); x = nullptr; }
#endif

enum class eDemoMode { Spinning3DCube, StereoImage };

void InitializeOffscreenFrameBuffer();

// Global Variables.
const wchar_t*                  g_windowTitle                  = L"CNSDK Getting Started D3D12 Sample";
const wchar_t*                  g_windowClass                  = L"CNSDKGettingStartedD3D12WindowClass";
int                             g_windowWidth                  = 1280;
int                             g_windowHeight                 = 720;
bool                            g_fullscreen                   = true;
leia::sdk::ILeiaSDK*            g_sdk                          = nullptr;
leia::sdk::IThreadedInterlacer* g_interlacer                   = nullptr;
eDemoMode                       g_demoMode                     = eDemoMode::Spinning3DCube;
float                           g_geometryDist                 = 500;
bool                            g_perspective                  = true;
float                           g_perspectiveCameraFiledOfView = 90.0f * 3.14159f / 180.0f;
float                           g_orthographicCameraHeight     = 500.0f;

// Global D3D12 Variables.
const int                     g_frameCount                        = 2;
ID3D12Device*                 g_device                            = nullptr;
ID3D12CommandQueue*           g_commandQueue                      = nullptr;
ID3D12CommandAllocator*       g_commandAllocator[g_frameCount]    = {};
IDXGISwapChain3*              g_swapChain                         = nullptr;
int                           g_frameIndex                        = 0;
DXGI_FORMAT                   g_swapChainFormat                   = DXGI_FORMAT_R8G8B8A8_UNORM;
ID3D12DescriptorHeap*         g_srvHeap                           = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE   g_srvFontCpuDescHandle              = {};
D3D12_GPU_DESCRIPTOR_HANDLE   g_srvFontGpuDescHandle              = {};
UINT                          g_srvDescriptorSize                 = 0;
UINT                          g_srvHeapUsed                       = 0;
ID3D12CommandAllocator*       g_guiCommandAllocator[g_frameCount] = {};
ID3D12GraphicsCommandList*    g_guiCommandList                    = nullptr;
ID3D12GraphicsCommandList*    g_textureUploadCommandList          = nullptr;
ID3D12Resource*               g_renderTargets[g_frameCount]       = {};
ID3D12DescriptorHeap*         g_rtvHeap                           = nullptr;
UINT                          g_rtvDescriptorSize                 = 0;
UINT                          g_rtvHeapUsed                       = 0;
CD3DX12_CPU_DESCRIPTOR_HANDLE g_renderTargetViews[g_frameCount]   = {};
ID3D12Resource*               g_depthStencil[g_frameCount]        = {};
ID3D12DescriptorHeap*         g_dsvHeap                           = nullptr;
UINT                          g_dsvDescriptorSize                 = 0;
UINT                          g_dsvHeapUsed                       = 0;
CD3DX12_CPU_DESCRIPTOR_HANDLE g_depthStencilViews[g_frameCount]   = {};
ID3D12Fence*                  g_fence                             = nullptr;
UINT64                        g_fenceValues[g_frameCount]         = {};
HANDLE                        g_fenceEvent                        = NULL;
ID3D12Resource*               g_offscreenTexture                  = nullptr;
CD3DX12_CPU_DESCRIPTOR_HANDLE g_offscreenShaderResourceView       = {};
CD3DX12_CPU_DESCRIPTOR_HANDLE g_offscreenRenderTargetView         = {};
ID3D12Resource*               g_offscreenDepthTexture             = nullptr;
CD3DX12_CPU_DESCRIPTOR_HANDLE g_offscreenDepthStencilView         = {};
const FLOAT                   g_offscreenColor[4]                 = { 0.0f, 0.2f, 0.5f, 1.0f };
const FLOAT                   g_backbufferColor[4]                = { 0.0f, 0.4f, 0.0f, 1.0f };
ID3D12Resource*               g_vertexBuffer                      = nullptr;
ID3D12Resource*               g_indexBuffer                       = nullptr;
D3D12_VERTEX_BUFFER_VIEW      g_vertexBufferView                  = {};
D3D12_INDEX_BUFFER_VIEW       g_indexBufferView                   = {};
D3D12_INPUT_LAYOUT_DESC       g_inputLayoutDesc                   = {};
ID3D12Resource*               g_constantBuffer[2]                 = {};
void*                         g_constantBufferDataBegin[2]        = {};
ID3D12GraphicsCommandList*    g_commandList                       = nullptr;
ID3D12GraphicsCommandList*    g_commandList2                      = nullptr;
ID3D12RootSignature*          g_rootSignature                     = nullptr;
ID3D12PipelineState*          g_pipelineState                     = nullptr;
ID3DBlob*                     g_compiledVertexShaderBlob          = nullptr;
ID3DBlob*                     g_compiledPixelShaderBlob           = nullptr;
ID3D12Resource*               g_imageTexture                      = nullptr;

#pragma pack(push, 1)

struct CONSTANTBUFFER
{
    mat4f transform;
};

struct VERTEX
{
    float pos[3];
    float color[3];

    VERTEX() = default;

    VERTEX(const float* p, const float* c)
    {
        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];
        color[0] = c[0];
        color[1] = c[1];
        color[2] = c[2];
    }

    VERTEX(float p0, float p1, float p2, float c0, float c1, float c2)
    {
        pos[0] = p0;
        pos[1] = p1;
        pos[2] = p2;
        color[0] = c0;
        color[1] = c1;
        color[2] = c2;
    }
};

#pragma pack(pop)

void OnError(const wchar_t* msg)
{
    MessageBox(NULL, msg, L"CNSDKGettingStartedD3D12", MB_ICONERROR | MB_OK); 
    exit(-1);
}

bool ReadEntireFile(const char* filename, bool binary, char*& data, size_t& dataSize)
{
    const int BUFFERSIZE = 4096;
    char buffer[BUFFERSIZE];

    // Open file.
    FILE* f = fopen(filename, binary ? "rb" : "rt");    
    if (f == NULL)
        return false;

    data     = nullptr;
    dataSize = 0;

    while (true)
    {
        // Read chunk into buffer.
        const size_t bytes = (int)fread(buffer, sizeof(char), BUFFERSIZE, f);
        if (bytes <= 0)
            break;

        // Extend allocated memory and copy chunk into it.
        char* newData = new char[dataSize + bytes];
        if (dataSize > 0)
        {
            memcpy(newData, data, dataSize);
            delete [] data;
            data = nullptr;
        }
        memcpy(newData + dataSize, buffer, bytes);
        dataSize += bytes;
        data = newData;
    }

    // Done and close.
    fclose(f);

    return dataSize > 0;
}

bool ReadTGA(const char* filename, int& width, int& height, GLint& format, char*& data, int& dataSize)
{
    char* ptr = nullptr;
    size_t fileSize = 0;
    if (!ReadEntireFile(filename, true, ptr, fileSize))
    {
        OnError(L"Failed to read TGA file.");
        return false;
    }

    static std::uint8_t DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    static std::uint8_t IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

    typedef union PixelInfo
    {
        std::uint32_t Colour;
        struct
        {
            std::uint8_t R, G, B, A;
        };
    } *PPixelInfo;

    // Read header.
    std::uint8_t Header[18] = { 0 };
    memcpy(&Header, ptr, sizeof(Header));
    ptr += sizeof(Header);

    int bitsPerPixel = 0;

    if (!std::memcmp(DeCompressed, &Header, sizeof(DeCompressed)))
    {
        bitsPerPixel = Header[16];
        width        = Header[13] * 256 + Header[12];
        height       = Header[15] * 256 + Header[14];
        dataSize     = ((width * bitsPerPixel + 31) / 32) * 4 * height;

        if ((bitsPerPixel != 24) && (bitsPerPixel != 32))
        {
            OnError(L"Invalid TGA file isn't 24/32-bit.");
            return false;
        }

        format = (bitsPerPixel == 24) ? GL_BGR : GL_BGRA;

        data = new char[dataSize];
        memcpy(data, ptr, dataSize);
    }
    else if (!std::memcmp(IsCompressed, &Header, sizeof(IsCompressed)))
    {
        bitsPerPixel = Header[16];
        width        = Header[13] * 256 + Header[12];
        height       = Header[15] * 256 + Header[14];
        dataSize     = width * height * sizeof(PixelInfo);

        if ((bitsPerPixel != 24) && (bitsPerPixel != 32))
        {
            OnError(L"Invalid TGA file isn't 24/32-bit.");
            return false;
        }

        format = (bitsPerPixel == 24) ? GL_BGR : GL_BGRA;

        PixelInfo Pixel = { 0 };
        int CurrentByte = 0;
        std::size_t CurrentPixel = 0;
        std::uint8_t ChunkHeader = { 0 };
        int BytesPerPixel = (bitsPerPixel / 8);

        data = new char[dataSize];

        do
        {
            memcpy(&ChunkHeader, ptr, sizeof(ChunkHeader));
            ptr += sizeof(ChunkHeader);

            if (ChunkHeader < 128)
            {
                ++ChunkHeader;
                for (int I = 0; I < ChunkHeader; ++I, ++CurrentPixel)
                {
                    memcpy(&Pixel, ptr, BytesPerPixel);
                    ptr += BytesPerPixel;

                    data[CurrentByte++] = Pixel.B;
                    data[CurrentByte++] = Pixel.G;
                    data[CurrentByte++] = Pixel.R;
                    if (bitsPerPixel > 24)
                        data[CurrentByte++] = Pixel.A;
                }
            }
            else
            {
                ChunkHeader -= 127;
                memcpy(&Pixel, ptr, BytesPerPixel);
                ptr += BytesPerPixel;

                for (int I = 0; I < ChunkHeader; ++I, ++CurrentPixel)
                {
                    data[CurrentByte++] = Pixel.B;
                    data[CurrentByte++] = Pixel.G;
                    data[CurrentByte++] = Pixel.R;
                    if (bitsPerPixel > 24)
                        data[CurrentByte++] = Pixel.A;
                }
            }
        } while (CurrentPixel < (width * height));
    }
    else
    {
        OnError(L"Invalid TGA file isn't 24/32-bit.");
        return false;
    }
   
    return true;
}

BOOL CALLBACK GetDefaultWindowStartPos_MonitorEnumProc(__in  HMONITOR hMonitor, __in  HDC hdcMonitor, __in  LPRECT lprcMonitor, __in  LPARAM dwData)
{
    std::vector<MONITORINFOEX>& infoArray = *reinterpret_cast<std::vector<MONITORINFOEX>*>(dwData);
    MONITORINFOEX info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    GetMonitorInfo(hMonitor, &info);
    infoArray.push_back(info);
    return TRUE;
}

bool GetNonPrimaryDisplayTopLeftCoordinate(int& x, int& y)
{
    // Get connected monitor info.
    std::vector<MONITORINFOEX> mInfo;
    mInfo.reserve(::GetSystemMetrics(SM_CMONITORS));
    EnumDisplayMonitors(NULL, NULL, GetDefaultWindowStartPos_MonitorEnumProc, reinterpret_cast<LPARAM>(&mInfo));

    // If we have multiple monitors, select the first non-primary one.
    if (mInfo.size() > 1)
    {
        for (int i = 0; i < mInfo.size(); i++)
        {
            const MONITORINFOEX& mi = mInfo[i];

            if (0 == (mi.dwFlags & MONITORINFOF_PRIMARY))
            {
                x = mi.rcMonitor.left;
                y = mi.rcMonitor.top;
                return true;
            }
        }
    }

    // Didn't find a non-primary, there is only one display connected.
    x = 0;
    y = 0;
    return false;
}

HWND CreateGraphicsWindow(HINSTANCE hInstance)
{
    // Create window.
    HWND hWnd = NULL;
    {
        int defaultX = 0;
        int defaultY = 0;
        GetNonPrimaryDisplayTopLeftCoordinate(defaultX, defaultY);

        DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;            // Window Extended Style
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;                            // Windows Style

        RECT WindowRect;
        WindowRect.left = (long)defaultX;
        WindowRect.right = (long)(defaultX + g_windowWidth);
        WindowRect.top = (long)defaultY;
        WindowRect.bottom = (long)(defaultY + g_windowHeight);
        //AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);        // Adjust Window To True Requested Size

        hWnd = CreateWindowEx
        (
            dwExStyle,
            g_windowClass,                         // Class Name
            g_windowTitle,                         // Window Title
            dwStyle |                              // Defined Window Style
            WS_CLIPSIBLINGS |                      // Required Window Style
            WS_CLIPCHILDREN,                       // Required Window Style
            WindowRect.left,                       // Window left
            WindowRect.top,                        // Window top
            WindowRect.right - WindowRect.left,    // Calculate Window Width
            WindowRect.bottom - WindowRect.top,    // Calculate Window Height
            NULL,                                  // No Parent Window
            NULL,                                  // No Menu
            hInstance,                             // Instance
            NULL                                   // Dont Pass Anything To WM_CREATE
        );

        if (!hWnd)
            OnError(L"Failed to create window.");
    }
    return hWnd;
}

void SetFullscreen(HWND hWnd, bool fullscreen)
{
    static int windowPrevX = 0;
    static int windowPrevY = 0;
    static int windowPrevWidth = 0;
    static int windowPrevHeight = 0;

    DWORD style = GetWindowLong(hWnd, GWL_STYLE);
    if (fullscreen)
    {
        RECT rect;
        MONITORINFO mi = { sizeof(mi) };
        GetWindowRect(hWnd, &rect);

        windowPrevX = rect.left;
        windowPrevY = rect.top;
        windowPrevWidth = rect.right - rect.left;
        windowPrevHeight = rect.bottom - rect.top;

        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        MONITORINFO mi = { sizeof(mi) };
        UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_NOTOPMOST, windowPrevX, windowPrevY, windowPrevWidth, windowPrevHeight, flags);
    }
}

void TransitionResourceState(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES fromState, D3D12_RESOURCE_STATES toState)
{
    assert(resource != nullptr);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, fromState, toState);
    commandList->ResourceBarrier(1, &barrier);
}

HRESULT ResizeBuffers(int width, int height)
{
    if (g_device == nullptr)
        return S_OK;

    for (int i = 0; i < g_frameCount; i++)
    {
        if (g_renderTargets[i] != nullptr)
        {
            g_renderTargets[i]->Release();
            g_renderTargets[i] = nullptr;
        }

        if (g_depthStencil[i] != nullptr)
        {
            g_depthStencil[i]->Release();
            g_depthStencil[i] = nullptr;
        }
    }

    g_rtvHeapUsed = 0;
    g_dsvHeapUsed = 0;
    g_srvHeapUsed = 0;

    memset(g_renderTargetViews, 0, sizeof(g_renderTargetViews));
    memset(g_depthStencilViews, 0, sizeof(g_depthStencilViews));

    // Resize swapchain.
    HRESULT hr = g_swapChain->ResizeBuffers(g_frameCount, width, height, g_swapChainFormat, 0);
    if (FAILED(hr))
        OnError(L"Failed to resize swapchain.");

    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (int n = 0; n < g_frameCount; n++)
        {
            g_renderTargetViews[n] = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            g_renderTargetViews[n].Offset(g_rtvHeapUsed);

            hr = g_swapChain->GetBuffer(n, _uuidof(ID3D12Resource), (void**) &g_renderTargets[n]);
            g_device->CreateRenderTargetView(g_renderTargets[n], nullptr, g_renderTargetViews[n]);
            
            g_rtvHeapUsed += g_rtvDescriptorSize;
        }
   
        // Create a depth stencil buffer and a DSV for each frame.
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags                         = D3D12_DSV_FLAG_NONE;

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format             = DXGI_FORMAT_D32_FLOAT;
            depthOptimizedClearValue.DepthStencil.Depth = 1.0f;

            for (int n = 0; n < g_frameCount; n++)
            {
                CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

                hr = g_device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    &depthOptimizedClearValue,
                    _uuidof(ID3D12Resource),
                    (void**)&g_depthStencil[n]);

                g_depthStencilViews[n] = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
                g_depthStencilViews[n].Offset(g_dsvHeapUsed);//1, g_dsvHeapUsed);

                g_device->CreateDepthStencilView(g_depthStencil[n], &depthStencilDesc, g_depthStencilViews[n]);

                g_dsvHeapUsed += g_dsvDescriptorSize;
            }
        }
    }

    //
    if (g_demoMode == eDemoMode::Spinning3DCube)
        InitializeOffscreenFrameBuffer();

    return S_OK;
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false)
{
    *ppAdapter = nullptr;

    IDXGIAdapter1* adapter = nullptr;

    IDXGIFactory6* factory6 = nullptr;
    if (SUCCEEDED(pFactory->QueryInterface(_uuidof(IDXGIAdapter1), (void**) &adapter)))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter;
}

// Prepare to render the next frame.
void MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = g_fenceValues[g_frameIndex];
    g_commandQueue->Signal(g_fence, currentFenceValue);

    // Update the frame index.
    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (g_fence->GetCompletedValue() < g_fenceValues[g_frameIndex])
    {
        g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent);
        WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    g_fenceValues[g_frameIndex] = currentFenceValue + 1;
}

// Wait for pending GPU work to complete.
void WaitForGpu()
{
    // Schedule a Signal command in the queue.
    g_commandQueue->Signal(g_fence, g_fenceValues[g_frameIndex]);

    // Wait until the fence has been processed.
    g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent);
    WaitForSingleObjectEx(g_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    g_fenceValues[g_frameIndex]++;
}

HRESULT InitializeD3D12(HWND hWnd)
{
    HRESULT hr = S_OK;

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(_uuidof(ID3D12Debug), (void**) &debugController)))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    IDXGIFactory4* factory = nullptr;
    hr = CreateDXGIFactory2(dxgiFactoryFlags, _uuidof(IDXGIFactory4), (void**) &factory);
    if(FAILED(hr))
        return hr;

    IDXGIAdapter1* hardwareAdapter = nullptr;
    GetHardwareAdapter(factory, &hardwareAdapter);

    hr = D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**) &g_device);
    if (FAILED(hr))
        return hr;
        
    // Get window size.
    RECT rc;
    GetClientRect(hWnd, &rc);
    const UINT width = rc.right - rc.left;
    const UINT height = rc.bottom - rc.top;

    // Create the command queue.
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        hr = g_device->CreateCommandQueue(&queueDesc, _uuidof(ID3D12CommandQueue), (void**)&g_commandQueue);
        if (FAILED(hr))
            return hr;
    }

    // Create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount      = g_frameCount;
    swapChainDesc.Width            = width;
    swapChainDesc.Height           = height;
    swapChainDesc.Format           = g_swapChainFormat;
    swapChainDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    IDXGISwapChain1* swapChain = nullptr;
    hr = factory->CreateSwapChainForHwnd(g_commandQueue, hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
    if (FAILED(hr))
        return hr;
    swapChain->QueryInterface(_uuidof(IDXGISwapChain3), (void**) &g_swapChain);// g_swapChain = dynamic_cast<IDXGISwapChain3*>(swapChain);
    if (g_swapChain == nullptr)
        return hr;

    // 
    hr = factory->MakeWindowAssociation(hWnd, 0);//DXGI_MWA_NO_ALT_ENTER
    if (FAILED(hr))
        return hr;

    g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 64;//g_frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = g_device->CreateDescriptorHeap(&rtvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**) &g_rtvHeap);
        g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Describe and create a depth stencil view (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 64;//g_frameCount;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        hr = g_device->CreateDescriptorHeap(&dsvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**)&g_dsvHeap);
        g_dsvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // Describe and create a shader resource view (SRV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 64;//g_frameCount;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = g_device->CreateDescriptorHeap(&srvHeapDesc, _uuidof(ID3D12DescriptorHeap), (void**)&g_srvHeap);
        g_srvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
        
    for (int i=0; i<g_frameCount; i++)
    {
        hr = g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, _uuidof(ID3D12CommandAllocator), (void**)&g_commandAllocator[i]);
        if (FAILED(hr))
            OnError(L"Failed to create command allocator.");

        hr = g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, _uuidof(ID3D12CommandAllocator), (void**)&g_guiCommandAllocator[i]);
        if (FAILED(hr))
            OnError(L"Failed to create command allocator.");
    }

    {
        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_frameIndex], NULL, _uuidof(ID3D12GraphicsCommandList), (void**)&g_commandList);
        if (FAILED(hr))
            OnError(L"Failed to create command list.");

        hr = g_commandList->Close();
        if (FAILED(hr))
            OnError(L"Failed to close command list.");

        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_frameIndex], NULL, _uuidof(ID3D12GraphicsCommandList), (void**)&g_commandList2);
        if (FAILED(hr))
            OnError(L"Failed to create command list.");

        hr = g_commandList2->Close();
        if (FAILED(hr))
            OnError(L"Failed to close command list.");
    }

    {
        // Create SRV for GUI font.
        g_srvFontCpuDescHandle = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
        g_srvFontGpuDescHandle = g_srvHeap->GetGPUDescriptorHandleForHeapStart();

        // Create command list.
        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_guiCommandAllocator[g_frameIndex], nullptr, _uuidof(ID3D12GraphicsCommandList), (void**) &g_guiCommandList);
        if (FAILED(hr))
        {
            assert(false);
        }

        hr = g_guiCommandList->Close();
        if (FAILED(hr))
        {
            assert(false);
        }
    }

    {
        // Create command list.
        hr = g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator[g_frameIndex], nullptr, _uuidof(ID3D12GraphicsCommandList), (void**)&g_textureUploadCommandList);
        if (FAILED(hr))
        {
            assert(false);
        }

        hr = g_textureUploadCommandList->Close();
        if (FAILED(hr))
        {
            assert(false);
        }
    }

    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (int n = 0; n < g_frameCount; n++)
        {
            g_renderTargetViews[n] = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            g_renderTargetViews[n].Offset(g_rtvHeapUsed);//1, g_rtvHeapUsed);

            hr = g_swapChain->GetBuffer(n, _uuidof(ID3D12Resource), (void**) &g_renderTargets[n]);
            g_device->CreateRenderTargetView(g_renderTargets[n], nullptr, g_renderTargetViews[n]);
            
            g_rtvHeapUsed += g_rtvDescriptorSize;
        }
   
        // Create a depth stencil buffer and a DSV for each frame.
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format                        = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags                         = D3D12_DSV_FLAG_NONE;

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format             = DXGI_FORMAT_D32_FLOAT;
            depthOptimizedClearValue.DepthStencil.Depth = 1.0f;

            for (int n = 0; n < g_frameCount; n++)
            {
                CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

                hr = g_device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    &depthOptimizedClearValue,
                    _uuidof(ID3D12Resource),
                    (void**)&g_depthStencil[n]);

                g_depthStencilViews[n] = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
                g_depthStencilViews[n].Offset(g_dsvHeapUsed);//1, g_dsvHeapUsed);

                g_device->CreateDepthStencilView(g_depthStencil[n], &depthStencilDesc, g_depthStencilViews[n]);

                g_dsvHeapUsed += g_dsvDescriptorSize;
            }
        }
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        hr = g_device->CreateFence(g_fenceValues[g_frameIndex], D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&g_fence);
        if (FAILED(hr))
            return hr;
        g_fenceValues[g_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (g_fenceEvent == nullptr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (FAILED(hr))
                return hr;
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }

    // Update render-target view.
    //hr = ResizeBuffersD3D12(renderer, width, height);
    //if (FAILED(hr))
      //  return hr;

    return S_OK;
}

void InitializeCNSDK(HWND hWnd)
{
    // Initialize SDK.
    g_sdk = leia::sdk::CreateLeiaSDK();
    leia::PlatformInitArgs pia;
    g_sdk->InitializePlatform(pia);
    g_sdk->Initialize(nullptr);

    // Initialize interlacer.
    leia::sdk::ThreadedInterlacerInitArgs interlacerInitArgs = {};
    interlacerInitArgs.useMegaTextureForViews = true;
    interlacerInitArgs.graphicsAPI = leia::sdk::GraphicsAPI::D3D12;
    g_interlacer = g_sdk->CreateNewThreadedInterlacer(interlacerInitArgs);
    g_interlacer->InitializeD3D12(g_device, g_commandQueue, leia::sdk::eLeiaTaskResponsibility::SDK, leia::sdk::eLeiaTaskResponsibility::SDK, leia::sdk::eLeiaTaskResponsibility::SDK);

    // Initialize interlacer GUI.
    leia::sdk::DebugMenuInitArgs debugMenuInitArgs;
    debugMenuInitArgs.gui.surface                   = hWnd;
    debugMenuInitArgs.gui.d3d12Device               = g_device;
    debugMenuInitArgs.gui.d3d12DeviceCbvSrvHeap     = g_srvHeap;
    debugMenuInitArgs.gui.d3d12FontSrvCpuDescHandle = *((uint64_t*)&g_srvFontCpuDescHandle);
    debugMenuInitArgs.gui.d3d12FontSrvGpuDescHandle = *((uint64_t*)&g_srvFontGpuDescHandle);
    debugMenuInitArgs.gui.d3d12NumFramesInFlight    = g_frameCount;
    debugMenuInitArgs.gui.d3d12RtvFormat            = g_swapChainFormat;
    debugMenuInitArgs.gui.d3d12CommandList          = g_guiCommandList;
    debugMenuInitArgs.gui.graphicsAPI               = leia::sdk::GuiGraphicsAPI::D3D12;
    g_interlacer->InitializeGui(debugMenuInitArgs);

    // Set stereo sliding mode.
    g_interlacer->SetInterlaceMode(leia::sdk::eLeiaInterlaceMode::StereoSliding);
    const int numViews = g_interlacer->GetNumViews();
    if (numViews != 2)
        OnError(L"Unexpected number of views");

    // Have to init this after a glContext is created but before we make any calls to OpenGL
    g_interlacer->InitOnDesiredThread();
}

void LoadScene()
{
    if (g_demoMode == eDemoMode::Spinning3DCube)
    {
        const float cubeWidth = 200.0f;
        const float cubeHeight = 200.0f;
        const float cubeDepth = 200.0f;

        const float l = -cubeWidth / 2.0f;
        const float r = l + cubeWidth;
        const float b = -cubeHeight / 2.0f;
        const float t = b + cubeHeight;
        const float n = -cubeDepth / 2.0f;
        const float f = n + cubeDepth;

        const int vertexCount = 8;
        const int indexCount  = 36;

        const float cubeVerts[vertexCount][3] =
        {
            {l, n, b}, // Left Near Bottom
            {l, f, b}, // Left Far Bottom
            {r, f, b}, // Right Far Bottom
            {r, n, b}, // Right Near Bottom
            {l, n, t}, // Left Near Top
            {l, f, t}, // Left Far Top
            {r, f, t}, // Right Far Top
            {r, n, t}  // Right Near Top
        };

        static const int faces[6][4] =
        {
            {0,1,2,3}, // bottom
            {1,0,4,5}, // left
            {0,3,7,4}, // front
            {3,2,6,7}, // right
            {2,1,5,6}, // back
            {4,7,6,5}  // top
        };

        static const float faceColors[6][3] =
        {
            {1,0,0},
            {0,1,0},
            {0,0,1},
            {1,1,0},
            {0,1,1},
            {1,0,1}
        };

        std::vector<VERTEX> vertices;
        std::vector<int> indices;
        for (int i = 0; i < 6; i++)
        {
            const int i0 = faces[i][0];
            const int i1 = faces[i][1];
            const int i2 = faces[i][2];
            const int i3 = faces[i][3];

            // Add indices.
            const int startIndex = (int)vertices.size();
            indices.emplace_back(startIndex + 0);
            indices.emplace_back(startIndex + 2);
            indices.emplace_back(startIndex + 1);
            indices.emplace_back(startIndex + 0);
            indices.emplace_back(startIndex + 3);
            indices.emplace_back(startIndex + 2);

            vertices.emplace_back(VERTEX(cubeVerts[i0], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i1], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i2], faceColors[i]));
            vertices.emplace_back(VERTEX(cubeVerts[i3], faceColors[i]));
        }        

        // Create vertex buffer.
        if (vertexCount > 0)
        {
            // Format = XYZ|RGB
            const int vertexSize       = sizeof(VERTEX);
            const int vertexBufferSize = vertices.size() * vertexSize;

            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            HRESULT hr = g_device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**) &g_vertexBuffer);

            if (FAILED(hr))
                OnError(L"Failed to create vertex buffer.");

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            hr = g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
            if (FAILED(hr))
                OnError(L"Failed to map vertex buffer.");

            memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
            g_vertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
            g_vertexBufferView.StrideInBytes  = vertexSize;
            g_vertexBufferView.SizeInBytes    = vertexBufferSize;
        }

        // Create index buffer.
        if (indexCount > 0)
        {
            // Format = int
            const int indexSize = sizeof(unsigned int);
            const int indexBufferSize = indexCount * indexSize;

            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            HRESULT hr = g_device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**) &g_indexBuffer);

            if (FAILED(hr))
                OnError(L"Failed to create index buffer.");

            // Copy the triangle data to the index buffer.
            UINT8* pIndexDataBegin;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            hr = g_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
            if (FAILED(hr))
                OnError(L"Failed to map index buffer.");

            memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
            g_indexBuffer->Unmap(0, nullptr);

            // Initialize the index buffer view.
            g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
            g_indexBufferView.SizeInBytes = indexBufferSize;
            g_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        }

        const char* vertexShaderText = 
            "struct VSInput\n"
            "{\n"
            "    float3 Pos : POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "struct PSInput\n"
            "{\n"
            "    float4 Pos : SV_POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "cbuffer ConstantBufferData : register(b0)\n"
            "{\n"
            "    float4x4 transform;\n"
            "};\n"
            "PSInput VSMain(VSInput input)\n"
            "{\n"
            "    PSInput output = (PSInput)0;\n"
            "    output.Pos = mul(transform, float4(input.Pos, 1.0f));\n"
            "    output.Col = input.Col;\n"
            "    return output;\n"
            "}\n";

	    const char* pixelShaderText =
            "struct PSInput\n"
            "{\n"
            "    float4 Pos : SV_POSITION;\n"
            "    float3 Col : COLOR;\n"
            "};\n"
            "float4 PSMain(PSInput input) : SV_Target0\n"
            "{\n"
            "    return float4(input.Col, 1);\n"
            "};\n";

        UINT compileFlags = 0;
#ifdef _DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        // Compile the vertex shader
        ID3DBlob* pVSErrors = nullptr;
        HRESULT hr = D3DCompile(vertexShaderText, strlen(vertexShaderText), NULL, NULL, NULL, "VSMain", "vs_5_0", compileFlags, 0, &g_compiledVertexShaderBlob, &pVSErrors);
        if (FAILED(hr))
        {
            std::string errorMsg;
            if (pVSErrors != nullptr)
            {
                errorMsg.append((char*)pVSErrors->GetBufferPointer(), pVSErrors->GetBufferSize());
                pVSErrors->Release();
            }
            OnError(L"Error creating vertex shader.");
        }

        // Compile the pixel shader        
        ID3DBlob* pPSErrors = nullptr;
        hr = D3DCompile(pixelShaderText, strlen(pixelShaderText), NULL, NULL, NULL, "PSMain", "ps_5_0", compileFlags, 0, &g_compiledPixelShaderBlob, &pPSErrors);
        if (FAILED(hr))
        {
            std::string errorMsg;
            if (pPSErrors != nullptr)
            {
                errorMsg.append((char*)pPSErrors->GetBufferPointer(), pPSErrors->GetBufferSize());
                pPSErrors->Release();
            }
            OnError(L"Error creating pixel shader.");
        }

        // Create vertex input layout.
        {
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            g_inputLayoutDesc.NumElements = _countof(inputElementDescs);
            g_inputLayoutDesc.pInputElementDescs = inputElementDescs;
        }

        {
            int shaderUniformBufferSize = sizeof(mat4f);

            int shaderUniformBufferSizeRounded = (shaderUniformBufferSize + 255) & ~255; // round to 256 bytes

            CD3DX12_HEAP_PROPERTIES heapPropsUpload(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(shaderUniformBufferSizeRounded);

            for (int i = 0; i < 2; i++)
            {
                HRESULT hr = g_device->CreateCommittedResource(
                    &heapPropsUpload,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    _uuidof(ID3D12Resource),
                    (void**)&g_constantBuffer[i]);

                if (FAILED(hr))
                    OnError(L"Failed to create constant buffer.");

                // Map and initialize the constant buffer. We don't unmap this until the
                // app closes. Keeping things mapped for the lifetime of the resource is okay.
                CD3DX12_RANGE readRange(0, 0);
                hr = g_constantBuffer[i]->Map(0, &readRange, reinterpret_cast<void**>(&g_constantBufferDataBegin[i]));
            }
        }

        // Create root signature.
        {
            D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
            rootCBVDescriptor.RegisterSpace  = 0;
            rootCBVDescriptor.ShaderRegister = 0;

            D3D12_ROOT_PARAMETER rootParameters = {};
            rootParameters.ParameterType    = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParameters.Descriptor       = rootCBVDescriptor;
            rootParameters.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

            // Create root signature.
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(1, &rootParameters, 0, nullptr, rootSignatureFlags);

            ID3DBlob* pErrorBlob = nullptr;
            ID3DBlob* signature = nullptr;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &pErrorBlob);

            if (FAILED(hr))
            {
                char msg[2048] = {};
                if (pErrorBlob != nullptr)
                    strncpy(msg, (const char*)pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
                OnError(L"Failed to serialize root signature.");
            }

            hr = g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));
            if (FAILED(hr))
                OnError(L"Failed to create root signature.");
        }

        // Create pipeline state object.
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout                     = g_inputLayoutDesc;//inputLayoutDesc;
            psoDesc.pRootSignature                  = g_rootSignature;
            psoDesc.VS                              = CD3DX12_SHADER_BYTECODE(g_compiledVertexShaderBlob);
            psoDesc.PS                              = CD3DX12_SHADER_BYTECODE(g_compiledPixelShaderBlob);
            psoDesc.RasterizerState                 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState                      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                    
            psoDesc.DepthStencilState               = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode        = D3D12_CULL_MODE_NONE;
            psoDesc.SampleDesc.Count                = 1;
            psoDesc.SampleDesc.Quality              = 0;
            psoDesc.SampleMask                      = UINT_MAX;
            psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets                = 1;
            psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat                       = DXGI_FORMAT_D32_FLOAT;
            psoDesc.SampleDesc.Count                = 1;

            HRESULT hr = g_device->CreateGraphicsPipelineState(&psoDesc, _uuidof(ID3D12PipelineState), (void**)&g_pipelineState);
            if (FAILED(hr))
                OnError(L"Failed to create pipeline state.");
        }

    }
    else if (g_demoMode == eDemoMode::StereoImage)
    {
        // Load stereo image.
        int width = 0;
        int height = 0;
        GLint format = 0;
        char* data = nullptr;
        int dataSize = 0;
        ReadTGA("StereoBeerGlass.tga", width, height, format, data, dataSize);

        // D3D11 doesn't support RGB textures, so expand initial data from RGB->RGBA.
        unsigned char* convertedInitialData = nullptr;
        {
            convertedInitialData = new unsigned char[width * height * 4];

            const unsigned char* pSrc = (const unsigned char*)data;
                  unsigned char* pDst = (unsigned char*)convertedInitialData;

            for (int i = 0; i < width * height; i++)
            {
                pDst[0] = pSrc[2];
                pDst[1] = pSrc[1];
                pDst[2] = pSrc[0];
                pDst[3] = 255;

                pDst += 4;
                pSrc += 3;
            }
        }

        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels          = 1;
        textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width              = width;
        textureDesc.Height             = height;
        textureDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize   = 1;
        textureDesc.SampleDesc.Count   = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = g_device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            _uuidof(ID3D12Resource),
            (void**) &g_imageTexture);

        if (FAILED(hr))
            OnError(L"Failed to create image texture.");

        {
            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_imageTexture, 0, 1);

            // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
            // the command list that references it has finished executing on the GPU.
            // We will flush the GPU at the end of this method to ensure the resource is not
            // prematurely destroyed.
            ID3D12Resource* textureUploadHeap = nullptr;
            
            CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);

            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            // Create the GPU upload buffer.
            hr = g_device->CreateCommittedResource(
                &heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                _uuidof(ID3D12Resource),
                (void**)&textureUploadHeap);

            if (FAILED(hr))
                OnError(L"Failed to create texture upload buffer");

            // Prepare command-list.
            {
                // Reset list.
                hr = g_textureUploadCommandList->Reset(g_commandAllocator[g_frameIndex], NULL);
                if (FAILED(hr))
                    OnError(L"Failed to reset texture upload command-list");
            }

            // Fill command-list.
            {
                D3D12_SUBRESOURCE_DATA textureData = {};
                textureData.pData      = convertedInitialData;
                textureData.RowPitch   = width * 4;
                textureData.SlicePitch = height * textureData.RowPitch;

                UpdateSubresources(g_textureUploadCommandList, g_imageTexture, textureUploadHeap, 0, 0, 1, &textureData);

                //TransitionShaderResource(g_textureUploadCommandList);

                TransitionResourceState(g_textureUploadCommandList, g_imageTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            }

            // Close command-list.
            HRESULT hr = g_textureUploadCommandList->Close();
            if (FAILED(hr))
                OnError(L"Failed to close texture upload command-list");

            // Execute command list.
            ID3D12CommandList* ppCommandLists[] = { g_textureUploadCommandList };
            g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
        }
    }

    // Wait for loading to complete.
    {
        // Create fence.
        ID3D12Fence* loadSceneFence = nullptr;
        HRESULT hr = g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, _uuidof(ID3D12Fence), (void**)&loadSceneFence);
        if (FAILED(hr))
            OnError(L"Failed to create load scene fence");

        // Create an event handle to use for frame synchronization.
        HANDLE loadSceneFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (loadSceneFenceEvent == nullptr)
            OnError(L"Failed to create load scene fence event");

        // Signal wait for fence event.
        g_commandQueue->Signal(loadSceneFence, 1);
        loadSceneFence->SetEventOnCompletion(1, loadSceneFenceEvent);
        WaitForSingleObject(loadSceneFenceEvent, INFINITE);

        // Release.
        loadSceneFence->Release();
        CloseHandle(loadSceneFenceEvent);
    }
}

void InitializeOffscreenFrameBuffer()
{
    // Create a single double-wide offscreen framebuffer. 
    // When rendering, we will do two passes, like a typical VR application.
    // On pass 1 we render to the left and on pass 2 we render to the right.
    
    // Use Leia's pre-defined view size (you can use a different size to suit your application).
    const int width  = g_sdk->GetViewWidth() * 2;
    const int height = g_sdk->GetViewHeight();

    {
        if (g_offscreenTexture != nullptr)
        {
            g_offscreenTexture->Release();
            g_offscreenTexture = nullptr;
        }

        if (g_offscreenDepthTexture != nullptr)
        {
            g_offscreenDepthTexture->Release();
            g_offscreenDepthTexture = nullptr;
        }    
    }

    //const D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;//(initialData != nullptr) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels          = 1;
    textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width              = width;
    textureDesc.Height             = height;
    textureDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    textureDesc.DepthOrArraySize   = 1;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Create resource.
    {
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Color[0] = g_offscreenColor[0];
        optimizedClearValue.Color[1] = g_offscreenColor[1];
        optimizedClearValue.Color[2] = g_offscreenColor[2];
        optimizedClearValue.Color[3] = g_offscreenColor[3];
        optimizedClearValue.Format   = textureDesc.Format;
        
        CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = g_device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            &optimizedClearValue,
            _uuidof(ID3D12Resource),
            (void**) &g_offscreenTexture);
        
        if (FAILED(hr))
            OnError(L"Failed to create offscreen frame buffer texture.");
    }

    // Create shader view.
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format                        = textureDesc.Format;
        srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip     = 0;
        srvDesc.Texture2D.MipLevels           = 1;
        srvDesc.Texture2D.PlaneSlice          = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        g_offscreenShaderResourceView = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_srvHeap->GetCPUDescriptorHandleForHeapStart());
        g_offscreenShaderResourceView.Offset(g_srvHeapUsed);

        g_device->CreateShaderResourceView(g_offscreenTexture, &srvDesc, g_offscreenShaderResourceView);

        g_srvHeapUsed += g_srvDescriptorSize;
    }

    // Create render-target view.
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Format               = textureDesc.Format;
        rtvDesc.Texture2D.MipSlice   = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        g_offscreenRenderTargetView = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        g_offscreenRenderTargetView.Offset(g_rtvHeapUsed);

        g_device->CreateRenderTargetView(g_offscreenTexture, &rtvDesc, g_offscreenRenderTargetView);

        g_rtvHeapUsed += g_rtvDescriptorSize;
    }

    // Modify description for a depth texture.
    textureDesc.Format = DXGI_FORMAT_D32_FLOAT;
    textureDesc.Flags  = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Create depth texture.
    {
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.DepthStencil.Depth = 1.0f;
        optimizedClearValue.Format             = textureDesc.Format;

        CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

        HRESULT hr = g_device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            _uuidof(ID3D12Resource),
            (void**)&g_offscreenDepthTexture);

        if (FAILED(hr))
            OnError(L"Failed to create depth texture");
    }

    // Create depth-stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Format             = textureDesc.Format;
        dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        UINT dsvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        UINT dsvHeapOffset = 0;//rendererD3D12->AcquireDSVSlot();
        g_offscreenDepthStencilView = CD3DX12_CPU_DESCRIPTOR_HANDLE(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        g_offscreenDepthStencilView.Offset(dsvHeapOffset);

        g_device->CreateDepthStencilView(g_offscreenDepthTexture, &dsvDesc, g_offscreenDepthStencilView);
    }
}

void RotateOrientation(mat3f& orientation, float x, float y, float z)
{
    mat3f rx, ry, rz;
    rx.setAxisAngleRotation(vec3f(1.0, 0.0, 0.0), x);
    ry.setAxisAngleRotation(vec3f(0.0, 1.0, 0.0), y);
    rz.setAxisAngleRotation(vec3f(0.0, 0.0, 1.0), z);
    orientation = orientation * (rx * ry * rz);
}

void Render(float elapsedTime) 
{
    const int   viewWidth   = g_sdk->GetViewWidth();
    const int   viewHeight  = g_sdk->GetViewHeight();
    const float aspectRatio = (float)viewWidth / (float)viewHeight;

    // Reset allocators.
    g_commandAllocator[g_frameIndex]->Reset();
    g_guiCommandAllocator[g_frameIndex]->Reset();

    // Reset command-list.
    g_commandList->Reset(g_commandAllocator[g_frameIndex], g_pipelineState);
    g_commandList->SetGraphicsRootSignature(g_rootSignature);

    // Prepare GUI command-list.
    {
        // Reset the list.
        g_guiCommandList->Reset(g_guiCommandAllocator[g_frameIndex], nullptr);

        // set constant buffer descriptor heap
        ID3D12DescriptorHeap* descriptorHeaps[] = { g_srvHeap };
        g_guiCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        
        // Transition to render-target.
        TransitionResourceState(g_guiCommandList, g_renderTargets[g_frameIndex], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
        
        // set render-target for GUI (required by imGui)
        g_guiCommandList->OMSetRenderTargets(1, &g_renderTargetViews[g_frameIndex], FALSE, nullptr);
    }

    // Transition swapchain render-target (final output) from common/present to render-target.
    TransitionResourceState(g_commandList, g_renderTargets[g_frameIndex], D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

    if (g_demoMode == eDemoMode::StereoImage)
    {
        // Clear back-buffer to green.
        g_commandList->ClearRenderTargetView(g_renderTargetViews[g_frameIndex], g_backbufferColor, 0, NULL);
        g_commandList->Close();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { g_commandList };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Perform interlacing.
        g_interlacer->SetSourceViewsSize(viewWidth, viewHeight, true);
        g_interlacer->DoPostProcessPicture(g_windowWidth, g_windowHeight, g_imageTexture, g_renderTargets[g_frameIndex]);
    }
    else if (g_demoMode == eDemoMode::Spinning3DCube)
    {
        // geometry transform.
        mat4f geometryTransform;
        {
            // Place cube at specified distance.
            vec3f geometryPos = vec3f(0, g_geometryDist, 0);

            mat3f geometryOrientation;
            geometryOrientation.setIdentity();
            RotateOrientation(geometryOrientation, 0.1f * elapsedTime, 0.2f * elapsedTime, 0.3f * elapsedTime);
            geometryTransform.create(geometryOrientation, geometryPos);
        }

        // Clear back-buffer to green.
        g_commandList->ClearRenderTargetView(g_renderTargetViews[g_frameIndex], g_backbufferColor, 0, NULL);
        g_commandList->ClearDepthStencilView(g_depthStencilViews[g_frameIndex], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

        // Transition offscreen render-target (intermediate stereo texture with views) from shader-input to render-target.
        TransitionResourceState(g_commandList, g_offscreenTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Clear offscreen render-target to blue
        g_commandList->ClearRenderTargetView(g_offscreenRenderTargetView, g_offscreenColor, 0, NULL);
        g_commandList->ClearDepthStencilView(g_offscreenDepthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

        // Render stereo views.
        for (int i = 0; i < 2; i++)
        {
            g_commandList->SetGraphicsRootConstantBufferView(0, g_constantBuffer[i]->GetGPUVirtualAddress());

            // Get camera properties.
            glm::vec3 camPos = glm::vec3(0, 0, 0);
            glm::vec3 camDir = glm::vec3(0, 1, 0);
            glm::vec3 camUp = glm::vec3(0, 0, 1);

            // Compute view position and projection matrix for view.
            vec3f viewPos = vec3f(0, 0, 0);
            mat4f cameraProjection;
            if (g_perspective)
            {
                glm::mat4 viewProjMat;
                glm::vec3 viewCamPos;
                g_interlacer->GetConvergedPerspectiveViewInfo(i, camPos, camDir, camUp, g_perspectiveCameraFiledOfView, aspectRatio, 1.0f, 10000.0f, &viewCamPos, &viewProjMat);
                cameraProjection = mat4f(viewProjMat);
                viewPos = vec3f(viewCamPos);
            }
            else
            {
                glm::mat4 viewProjMat;
                glm::vec3 viewCamPos;
                g_interlacer->GetConvergedOrthographicViewInfo(i, camPos, camDir, camUp, g_orthographicCameraHeight * aspectRatio, g_orthographicCameraHeight, 1.0f, 10000.0f, &viewCamPos, &viewProjMat);
                cameraProjection = mat4f(viewProjMat);
                viewPos = vec3f(viewCamPos);
            }

            // Get camera transform.
            mat4f cameraTransform;
            cameraTransform.lookAt(camPos, camPos + camDir, vec3f(0.0f, 0.0f, 1.0f));

            // Compute combined matrix.
            const mat4f mvp = cameraProjection * cameraTransform * geometryTransform;

            // Set viewport to render to left, then right.
            CD3DX12_VIEWPORT viewport = {};
            viewport.TopLeftX = (FLOAT)(i * viewWidth);
            viewport.TopLeftY = 0.0f;
            viewport.Width    = (FLOAT)viewWidth;
            viewport.Height   = (FLOAT)viewHeight;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            g_commandList->RSSetViewports(1, &viewport);

            // Set scissor.
            D3D12_RECT scissorRect;
            scissorRect.left   = i * viewWidth;
            scissorRect.top    = 0;
            scissorRect.right  = scissorRect.left + viewWidth;
            scissorRect.bottom = scissorRect.top + viewHeight;
            g_commandList->RSSetScissorRects(1, &scissorRect);

            g_commandList->OMSetRenderTargets(1, &g_offscreenRenderTargetView, FALSE, &g_offscreenDepthStencilView);
            g_commandList->IASetIndexBuffer(&g_indexBufferView);
            g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
            g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                       
            memcpy(g_constantBufferDataBegin[i], &mvp, sizeof(mvp));

            g_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
        }

        // Set viewport to render to left, then right.
        CD3DX12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width    = (FLOAT)g_windowWidth;
        viewport.Height   = (FLOAT)g_windowHeight;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        g_commandList->RSSetViewports(1, &viewport);

        // Set scissor.
        D3D12_RECT scissorRect;
        scissorRect.left   = 0;
        scissorRect.top    = 0;
        scissorRect.right  = g_windowWidth;
        scissorRect.bottom = g_windowHeight;
        g_commandList->RSSetScissorRects(1, &scissorRect);

        // Transition offscreen texture to a shader input.
        TransitionResourceState(g_commandList, g_offscreenTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

        g_commandList->Close();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { g_commandList };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Perform interlacing.
        g_interlacer->SetSourceViewsSize(viewWidth, viewHeight, true);
        g_interlacer->SetInterlaceViewTextureAtlas(g_offscreenTexture);
        g_interlacer->DoPostProcess(g_windowWidth, g_windowHeight, false, g_renderTargets[g_frameIndex]);
    }

    // Transition render-target to be presentable.
    {
        // Record command-list to transition.
        g_commandList2->Reset(g_commandAllocator[g_frameIndex], nullptr);
        TransitionResourceState(g_commandList2, g_renderTargets[g_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_commandList2->Close();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { g_commandList2 };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }

    // Render GUI.
    {
        TransitionResourceState(g_guiCommandList, g_renderTargets[g_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        // Close gui commandlist.
        g_guiCommandList->Close();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { g_guiCommandList };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    }

    g_swapChain->Present(0, 0);

    MoveToNextFrame();
}

void UpdateWindowTitle(HWND hWnd, double curTime) 
{
    static double prevTime = 0;
    static int frameCount = 0;

    frameCount++;

    if (curTime - prevTime > 0.25) 
    {
        const double fps = frameCount / (curTime - prevTime);

        wchar_t newWindowTitle[128];
        swprintf(newWindowTitle, 128, L"%s (%.1f FPS)", g_windowTitle, fps);
        SetWindowText(hWnd, newWindowTitle);

        prevTime = curTime;
        frameCount = 0;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Allow CNSDK debug menu to see window messages
    if (g_interlacer != nullptr)
    {
        auto io = g_interlacer->ProcessGuiInput(hWnd, message, wParam, lParam);
        if (io.wantCaptureInput)
            return 0;
    }

    switch (message)
    {

    // Handle keypresses.
    case WM_KEYDOWN:
        switch (wParam) {
            case VK_ESCAPE:
                PostQuitMessage(0);
                break;
        }
        break;

    // Keep track of window size.
    case WM_SIZE:
        g_windowWidth = LOWORD(lParam);
        g_windowHeight = HIWORD(lParam);
        ResizeBuffers(g_windowWidth, g_windowHeight);
        PostMessage(hWnd, WM_PAINT, 0, 0);
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Register window class.
    WNDCLASSEXW wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CNSDKGETTINGSTARTEDD3D11));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = g_windowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassExW(&wcex);

    // Create window.
    HWND hWnd = CreateGraphicsWindow(hInstance);

    // Get DC.
    HDC hDC = GetDC(hWnd);

    // Switch to fullscreen if requested.
    if (g_fullscreen)
        SetFullscreen(hWnd, true);

    // Initialize graphics.
    HRESULT hr = InitializeD3D12(hWnd);
    if (FAILED(hr))
        OnError(L"Failed to initialize D3D11");

    // Initialize CNSDK.
    InitializeCNSDK(hWnd);

    // Create our stereo (double-wide) frame buffer.
    if (g_demoMode == eDemoMode::Spinning3DCube)
        InitializeOffscreenFrameBuffer();

    // Prepare everything to draw.
    LoadScene();

    // Show window.
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    // Enable Leia display backlight.
    g_sdk->SetBacklight(true);

    // Main loop.
    bool finished = false;
    while (!finished)
    {
        // Empty all messages from queue.
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
            {
                finished = true;
                break;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // Perform app logic.
        if (!finished)
        {
            // Get timing.
            const ULONGLONG  curTick      = GetTickCount64();
            static ULONGLONG prevTick     = curTick;
            const ULONGLONG  elapsedTicks = curTick - prevTick;
            const double     elapsedTime  = (double)elapsedTicks / 1000.0;
            const double     curTime      = (double)curTick / 1000.0;

            // Render.
            Render((float)elapsedTime);

            // Update window title with FPS.
            UpdateWindowTitle(hWnd, curTime);
             
            //Sleep(0);
        }
    }

    // Disable Leia display backlight.
    g_sdk->SetBacklight(false);

    // Cleanup.
    g_sdk->Destroy();
    
    CloseHandle(g_fenceEvent);

    for (int i = 0; i < g_frameCount; i++)
    {
        SAFE_RELEASE(g_constantBuffer[i]);
        SAFE_RELEASE(g_depthStencil[i]);
        SAFE_RELEASE(g_renderTargets[i]);
        SAFE_RELEASE(g_guiCommandAllocator[i]);
        SAFE_RELEASE(g_commandAllocator[i]);
    }

    SAFE_RELEASE(g_imageTexture);
    SAFE_RELEASE(g_compiledPixelShaderBlob);
    SAFE_RELEASE(g_compiledVertexShaderBlob);
    SAFE_RELEASE(g_pipelineState);
    SAFE_RELEASE(g_rootSignature);
    SAFE_RELEASE(g_commandList2);
    SAFE_RELEASE(g_commandList);    
    SAFE_RELEASE(g_indexBuffer);
    SAFE_RELEASE(g_vertexBuffer);
    SAFE_RELEASE(g_offscreenDepthTexture);
    SAFE_RELEASE(g_offscreenTexture);
    SAFE_RELEASE(g_fence);
    SAFE_RELEASE(g_dsvHeap);    
    SAFE_RELEASE(g_rtvHeap);    
    SAFE_RELEASE(g_textureUploadCommandList);
    SAFE_RELEASE(g_guiCommandList);    
    SAFE_RELEASE(g_srvHeap);
    SAFE_RELEASE(g_swapChain);    
    SAFE_RELEASE(g_commandQueue);
    SAFE_RELEASE(g_device);

    return 0;
}