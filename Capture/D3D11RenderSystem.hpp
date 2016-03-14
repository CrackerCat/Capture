#ifndef D3D11_RenderSystem_Hpp
#define D3D11_RenderSystem_Hpp

#include <d3d11.h>
#include <dxgi.h>
#include "CaptureHUB.h"

namespace h3d {
	class H3D_API D3D11Texture : public CaptureTexture {
	public:
		D3D11Texture(ID3D11Texture2D* tex);

		~D3D11Texture();

		SDst GetWidth() const override;
		SDst GetHeight() const override;

		MappedData Map() override;
		void UnMap() override;

		void Assign(D3D11Texture*);
	private:
		D3D11_TEXTURE2D_DESC info;
		ID3D11Texture2D* texture;

		friend class D3D11Factory;
		friend class D3D11Engine;
	};

	class H3D_API RenderFactory{};

	class H3D_API D3D11Factory :public RenderFactory {
	public:
		D3D11Factory();

		D3D11Texture* CreateTexture(SDst Width, SDst Height, unsigned __int64 Handle);
		D3D11Texture* CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format);
	private:
		ID3D11Device* device;

		friend class D3D11Engine;
	};

	class H3D_API RenderEngine{};

	class H3D_API D3D11Engine : public RenderEngine {
	public:
		bool Construct(HWND hwnd);
		void Destroy();
	public:
		D3D11Engine();

		D3D11Factory& GetFactory();

		//todo convert format
		void CopyTexture(D3D11Texture* dst, D3D11Texture* src);

		D3D11Texture::MappedData Map(D3D11Texture*);
		void UnMap(D3D11Texture*);
	private:
		ID3D11DeviceContext* context;
		IDXGISwapChain* swap_chain;
		D3D11Factory* factory;
	};

	H3D_API D3D11Engine&  GetEngine();
}


#endif
