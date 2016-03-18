#ifndef D3D9_OEPNGL_GPU_HOOK
#define D3D9_OPENGL_GPU_HOOK

#include <d3d11.h>

namespace h3d {
	HANDLE CreateSharedTexture(_Outptr_ ID3D11Texture2D*& texture, _Outptr_ ID3D11Device*& device, _Outptr_ ID3D11DeviceContext* &context, DXGI_FORMAT format, UINT width, UINT height);
}

#endif
