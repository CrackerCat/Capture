#ifndef VIDEOENGINECAPTURHOOK_DXGICOMMON_H
#define VIDEOENGINECAPTURHOOK_DXGICOMMON_H

#include <d3d10_1.h>
#include <d3d11.h>
#include <dxgi.h>

#include "../Capture/CaptureHUB.h"
#include "Extern.h"
#include "../Capture/WinPlatform.h"

namespace h3d {

	void D3D11Capture(IDXGISwapChain *swap);
	void D3D11Flush();
	void D3D11CaptureSetup(IDXGISwapChain*);

	void D3D10Capture(IDXGISwapChain*);
	void D3D10Flush();
	void D3D10CaptureSetup(IDXGISwapChain*);

	//todo :check format

	//A Render-Target-View Cannote use the following formats
	//Any typelsee format.
	//DXGI_FORMAT_R32G32B32
	//https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064(v=vs.85).aspx
	inline SWAPFORMAT ConvertFormat(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return RGBA8;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return BGRA8;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return RGBA16;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return R10G10B10A2;
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			return R10G10B10XRA2;
		}
		return SWAPFORMAT_UNKNOWN;
	}

	inline DXGI_FORMAT DecayFormat(DXGI_FORMAT format) {
		//back buffer can't be srgb
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		//must go this;
		return format;
	}

	inline void CapturSetup(CaptureInfo& info, DXGI_FORMAT& format,bool & open_msaa,IDXGISwapChain* pSwapChain) {
		DXGI_SWAP_CHAIN_DESC swapDesc;
		if (SUCCEEDED(pSwapChain->GetDesc(&swapDesc))) {
			info.Reserved1 = ConvertFormat(DecayFormat(swapDesc.BufferDesc.Format));
			if (info.Reserved1 != SWAPFORMAT_UNKNOWN) {
				DXGI_MODE_DESC & BufferDesc = swapDesc.BufferDesc;
				if (format != DecayFormat(BufferDesc.Format) ||
					info.oWidth != BufferDesc.Width ||
					info.oHeight != BufferDesc.Height ||
					reinterpret_cast<HWND>(info.sNative) != swapDesc.OutputWindow) {
					format = DecayFormat(BufferDesc.Format);
					info.oWidth = BufferDesc.Width;
					info.oHeight = BufferDesc.Height;
					info.sNative = reinterpret_cast<unsigned __int64>(swapDesc.OutputWindow);

					open_msaa = swapDesc.SampleDesc.Count > 1;

					logstream << "Format : " << UINT(format) << "Resolution : " << BufferDesc.Width << "x" << BufferDesc.Height;
					logstream << "MSAA :" << std::boolalpha << open_msaa << std::endl;
				}
			}
		}
	}
}

#endif