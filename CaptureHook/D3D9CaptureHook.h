#ifndef D3D9_CAPTURE_HOOK
#define D3D9_CAPTURE_HOOK

#include <d3d9.h>
#include <dxgi.h>

void Flush();


bool GPUCaptureCheck(IDirect3DDevice9 * device);

void CreateGPUCapture(IDirect3DDevice9* device);
void D3D9CaptureGPU(IDirect3DDevice9* device);
void FlushGPU();

//COMMON
extern bool has_textured;
extern h3d::CaptureInfo d3d9_captureinfo;
extern HMODULE hD3D9Dll;
extern DWORD d3d9_format;

//GPU
extern DXGI_FORMAT d3d11_format;

#endif
