#ifndef D3D11_RenderSystem_Hpp
#define D3D11_RenderSystem_Hpp

#include <d3d11.h>
#include <dxgi.h>
#include "CaptureHUB.h"

#include <map>

namespace h3d {
#pragma warning(disable:4251)
	enum EAcess {
		EA_CPU_READ = 1 << 0,
		EA_CPU_WRITE = 1 << 1,
		EA_GPU_READ = 1 << 2,
		EA_GPU_WRITE = 1 << 3,
	};

	//todo 这个类应该继承GDITexture
	class H3D_API D3D11Texture : public CaptureTexture{
	public:
		D3D11Texture(ID3D11Texture2D* tex);

		virtual ~D3D11Texture();

		SDst GetWidth() const override;
		SDst GetHeight() const override;

		virtual MappedData Map() override;
		virtual void UnMap() override;

		void Assign(D3D11Texture*);

		ID3D11ShaderResourceView* RetriveD3DShaderResouceView();
		ID3D11RenderTargetView* RetriveD3DRenderTargetView();
	private:
		D3D11_TEXTURE2D_DESC info;
		ID3D11Texture2D* texture;

		::std::map<size_t, ID3D11ShaderResourceView*> d3d_srvs;
		typedef ::std::map<size_t, ID3D11ShaderResourceView*>::iterator srvmap_iter;

		ID3D11RenderTargetView* d3d_rtv;

		friend class D3D11Factory;
		friend class D3D11Engine;
	};

	class D3D11GDITexture :public h3d::D3D11Texture, public h3d::IGDI {
	public:
		D3D11GDITexture(ID3D11Texture2D* tex)
			:D3D11Texture(tex),surface(NULL) {
			tex->QueryInterface(&surface);
		}

		~D3D11GDITexture() {
			surface->Release();
		}

		HDC GetDC() override {
			HDC dc = NULL;
			surface->GetDC(TRUE, &dc);
			return dc;
		}

		void ReleaseDC() override {
			surface->ReleaseDC(NULL);
		}

		MappedData Map() override {
			return{NULL,0 };
		}
		void UnMap() override {

		}
	private:
		IDXGISurface1* surface;
	};

	class H3D_API RenderFactory{};

	class H3D_API D3D11Factory :public RenderFactory {
	public:
		D3D11Factory();

		D3D11Texture* CreateGDITexture(SDst Width, SDst Height);
		D3D11Texture* CreateTexture(SDst Width, SDst Height, unsigned __int64 Handle);
		D3D11Texture* CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format,unsigned int access);

		ID3D11ShaderResourceView* CreateSRV(ID3D11Texture2D* texture, const D3D11_SHADER_RESOURCE_VIEW_DESC & desc);
		ID3D11RenderTargetView* CreateRTV(ID3D11Texture2D*, const D3D11_RENDER_TARGET_VIEW_DESC* desc);
	private:
		ID3D11Device* device;

		friend class D3D11Engine;
	};

	class H3D_API RenderEngine{};

	enum BLEND_TYPE {
		OVERLAY_DRAW,
		ALPHA_DRAW
	};

	class H3D_API D3D11Engine : public RenderEngine {
	public:
		bool Construct(HWND hwnd);
		void Destroy();
	public:
		D3D11Engine();

		D3D11Factory& GetFactory();

		unsigned int GetLevel();

		void CopyTexture(D3D11Texture* dst, D3D11Texture* src);
		void ResloveTexture(D3D11Texture* dst, D3D11Texture* src);

		void BeginDraw(D3D11Texture* rt, BLEND_TYPE bt);
		void Draw(SDst x, SDst y, SDst width, SDst height, D3D11Texture* src);
		void EndDraw();

		D3D11Texture::MappedData Map(D3D11Texture*);
		void UnMap(D3D11Texture*);
	private:
		ID3D11DeviceContext* context;
		IDXGISwapChain* swap_chain;
		D3D11Factory* factory;

		ID3D11RenderTargetView* rt;

		unsigned int support_level;

		struct {
			ID3D11Buffer* reslove_vb;
			UINT vb_stride;
			UINT vb_offset;
			ID3D11InputLayout* reslove_il;
			ID3D11VertexShader* reslove_vs;
			ID3D11PixelShader* reslove_ps;
			ID3D11SamplerState* reslove_ps_ss;
		};
	};

	H3D_API D3D11Engine&  GetEngine();
}


#endif
