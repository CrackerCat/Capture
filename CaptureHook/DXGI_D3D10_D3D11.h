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
	inline SWAPFORMAT ConvertFormat(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return BGRA8;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return BGRX8;
		case DXGI_FORMAT_B5G6R5_UNORM:
			return B5G6R5;
		}
		return SWAPFORMAT_UNKNOWN;
	}

	inline DXGI_FORMAT DecayFormat(DXGI_FORMAT format) {
		switch (format)
		{
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		return format;
	}

	inline void CapturSetup(CaptureInfo& info, DXGI_FORMAT& format,bool & open_msaa,IDXGISwapChain* pSwapChain) {
		DXGI_SWAP_CHAIN_DESC swapDesc;
		if (SUCCEEDED(pSwapChain->GetDesc(&swapDesc))) {
			info.Reserved1 = ConvertFormat(swapDesc.BufferDesc.Format);
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