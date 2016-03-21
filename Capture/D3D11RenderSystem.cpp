#include "D3D11RenderSystem.hpp"
#include "WinPlatform.h"
#include <stdexcept>
#include <fstream>

#ifndef _USING_V110_SDK71_
#include <dxgi1_2.h>
#endif

typedef HRESULT(WINAPI*DXGICREATEPROC)(REFIID riid, void ** ppFactory);
typedef HRESULT(WINAPI*D3D11PROC)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, D3D_FEATURE_LEVEL*, UINT, UINT, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL*, ID3D11DeviceContext** ppImmediateContext);


ID3D10Device1* pDevice = NULL;


#include <vector>
std::vector<char> ReadShaderFile(const wchar_t* path) {
	std::ifstream fin;
	fin.open(path, std::ios_base::binary);
	fin.seekg(0, std::ios_base::end);
	size_t size = fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> buffer(size);
	fin.read(&buffer[0], size);
	fin.close();
	return buffer;
}

//vs2008 xp hook
//不能链接到D3D11.lib，dxgi.lib 只能采取动态载入其dll
//不支持vista sp1！
bool h3d::D3D11Engine::Construct(HWND hwnd)
{
	factory = new D3D11Factory();
	support_level = 0x9000;
	if (!factory)
		return false;

	HMODULE hDXGIDll = LoadLibraryW(L"dxgi.dll");
	if (!hDXGIDll)
		return false;

	int version = GetOSVersion();
	//7 CreateDXGIFactory1
	//8 CreateDXGIFactory2
#ifndef _USING_V110_SDK71_
	REFIID iid = version >= 8 ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);
#else
	REFIID iid = __uuidof(IUnknown);//xp不应该执行成功
#endif

	DXGICREATEPROC CreateDXGI = (DXGICREATEPROC)GetProcAddress(hDXGIDll, "CreateDXGIFactory1");

	IDXGIFactory1* pFactory = NULL;
	if (FAILED(CreateDXGI(iid, reinterpret_cast<void**>(&pFactory))))
		return false;

	//总是使用第一个显卡
	IDXGIAdapter1* pAdapter = NULL;
	if (FAILED(pFactory->EnumAdapters1(0, &pAdapter)))
		return false;

	//TODO log显卡信息，NVDIA hack

	RECT rect;
	GetWindowRect(hwnd, &rect);

	DXGI_SWAP_CHAIN_DESC swap_desc;
	ZeroMemory(&swap_desc, sizeof(swap_desc));
	swap_desc.BufferCount = 2;
	swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swap_desc.BufferDesc.Width = rect.right - rect.left;
	swap_desc.BufferDesc.Height = rect.bottom - rect.top;
	swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_desc.OutputWindow = hwnd;
	swap_desc.SampleDesc.Count = 1;
	swap_desc.SampleDesc.Quality = 0;
	swap_desc.Windowed = TRUE;
	swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_desc.Flags = 0;


	HMODULE hD3D11Dll = LoadLibraryW(L"d3d11.dll");
	if (!hD3D11Dll)
		return false;

	D3D11PROC create = (D3D11PROC)GetProcAddress(hD3D11Dll, "D3D11CreateDeviceAndSwapChain");
	if (!create)
		return false;

	D3D_FEATURE_LEVEL desired_levels[] = { D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2 ,D3D_FEATURE_LEVEL_9_1 };
	D3D_FEATURE_LEVEL support_feature;
	D3D_DRIVER_TYPE driver_type = D3D_DRIVER_TYPE_UNKNOWN;

	UINT Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	//Warning: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476083(v=vs.85).aspx
	//if pAdapter = NULL,driver_type = D3D_DRIVER_TYPE_NULL;create will be success,but the device can't OpenSharedResource
	//NULL是用于debug非渲染API/OpenSharedResource 将会失败
	//if pAdapter != NULL,driver_type != D3D_DRIVER_TYPE_UNKNOWN;create will be failed
	//推荐使用 pAdapter = NULL,driver_type = D3D_DRIVER_TYPE_HARDWARE
	//或者枚举显卡 pAdapter,driver_type = D3D_DRIVER_TYPE_UNKNOWN
	if (FAILED(
		(*create)(pAdapter, driver_type, NULL,Flags, desired_levels, 6, D3D11_SDK_VERSION, &swap_desc, &swap_chain, &factory->device, &support_feature, &context)
		))
		return false;

	support_level = static_cast<unsigned int>(support_feature);
	ID3D11Texture2D* back_buffer = NULL;
	swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));

	factory->device->CreateRenderTargetView(back_buffer, NULL, &rt);

	{
		std::vector<char> buffer = ReadShaderFile(L"Shader/ResloveVS.cso");
		D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		factory->device->CreateVertexShader(&buffer[0], buffer.size(), NULL, &reslove_vs);
		factory->device->CreateInputLayout(layout_desc, 1, &buffer[0], buffer.size(), &reslove_il);

		buffer.swap(ReadShaderFile(L"Shader/ReslovePS.cso"));
		factory->device->CreatePixelShader(&buffer[0], buffer.size(), NULL, &reslove_ps);
	}

	float vertices[4 * 4] = {
		1,1,1,1,
		1,-1,1,1,
		-1,1,1,1,
		-1,-1,1,1
	};
	vb_stride = 16;
	vb_offset = 0;

	CD3D11_BUFFER_DESC vbDesc(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
	D3D11_SUBRESOURCE_DATA resDesc;
	resDesc.pSysMem = vertices;
	factory->device->CreateBuffer(&vbDesc, &resDesc, &reslove_vb);

	CD3D11_SAMPLER_DESC point_sampler(D3D11_DEFAULT);
	point_sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	factory->device->CreateSamplerState(&point_sampler, &reslove_ps_ss);

	back_buffer->Release();
	pAdapter->Release();
	pFactory->Release();

	return true;
}

#define SR(var) if(var) {var->Release();var = NULL;}

void h3d::D3D11Engine::Destroy()
{
	SR(reslove_il);
	SR(reslove_vb);
	SR(reslove_vs);
	SR(reslove_ps);
	SR(reslove_ps_ss);


	SR(context);
	SR(swap_chain);
	SR(rt);
	SR(factory->device);
	delete factory;
}

h3d::D3D11Engine::D3D11Engine()
	:factory(NULL),context(NULL),swap_chain(NULL),rt(NULL)
{
}

h3d::D3D11Factory & h3d::D3D11Engine::GetFactory()
{
	return *factory;
}

unsigned int h3d::D3D11Engine::GetLevel()
{
	return support_level;
}

void h3d::D3D11Engine::CopyTexture(D3D11Texture * dst, D3D11Texture * src)
{
	context->CopyResource(dst->texture, src->texture);
}

void h3d::D3D11Engine::ResloveTexture(D3D11Texture * dst, D3D11Texture * src) {
	context->IASetVertexBuffers(0, 1, &reslove_vb, &vb_stride, &vb_offset);
	context->IASetInputLayout(reslove_il);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->VSSetShader(reslove_vs, NULL, 0);
	context->PSSetShader(reslove_ps, NULL, 0);
	context->PSSetSamplers(0, 1, &reslove_ps_ss);

	ID3D11RenderTargetView* dst_rtv = dst->RetriveD3DRenderTargetView();
	ID3D11ShaderResourceView* src_srv = src->RetriveD3DShaderResouceView();

	context->PSSetShaderResources(0, 1, &src_srv);
	context->OMSetRenderTargets(1, &dst_rtv, NULL);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Height =static_cast<FLOAT>(dst->info.Height);
	vp.Width = static_cast<FLOAT>(dst->info.Width);
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	context->Draw(4, 0);
}

#include <assert.h>
D3D11_MAP Mapping(UINT cpu_access) {
	switch (cpu_access)
	{
	case D3D11_CPU_ACCESS_READ:
		return D3D11_MAP_READ;
	case D3D11_CPU_ACCESS_WRITE:
		return D3D11_MAP_WRITE_DISCARD;
	case D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE:
		return D3D11_MAP_READ_WRITE;
	default:
		assert(false);
		return D3D11_MAP_READ;
	}
}

h3d::D3D11Texture::MappedData h3d::D3D11Engine::Map(D3D11Texture * tex)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	if (FAILED(context->Map(tex->texture, 0, Mapping(tex->info.CPUAccessFlags), 0, &subRes)))
		throw std::runtime_error("D3D11Texture2D Map Failed");
	return {reinterpret_cast<byte*>(subRes.pData),subRes.RowPitch };
}

void h3d::D3D11Engine::UnMap(D3D11Texture * tex)
{
	context->Unmap(tex->texture, 0);
}

h3d::D3D11Factory::D3D11Factory()
	:device(NULL)
{
}

h3d::D3D11Texture * h3d::D3D11Factory::CreateTexture(SDst Width, SDst Height, unsigned __int64 Handle)
{
	HANDLE hShare = reinterpret_cast<HANDLE>(Handle);

	ID3D11Resource* share_res = NULL;

	if (SUCCEEDED(device->OpenSharedResource(hShare, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&share_res)))) {
		ID3D11Texture2D* share_tex = NULL;
		if (SUCCEEDED(share_res->QueryInterface(&share_tex))) {
			share_res->Release();
			 return new D3D11Texture(share_tex);
		}
	}
	return nullptr;
}

DXGI_FORMAT ConvertFormat(h3d::SWAPFORMAT format) {
	switch (format) {
	case h3d::R10G10B10A2:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case h3d::BGRA8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case h3d::BGRX8:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case h3d::B5G6R5A1: case h3d::B5G6R5X1:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case h3d::B5G6R5:
		return DXGI_FORMAT_B5G6R5_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

#pragma warning(disable:4244)
h3d::D3D11Texture * h3d::D3D11Factory::CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format,unsigned int access)
{
	CD3D11_TEXTURE2D_DESC texDesc(ConvertFormat(Format),Width, Height, 1, 1, 0);

	if (access == (EA_CPU_WRITE | EA_GPU_READ))
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
	else {
		if (access == EA_CPU_WRITE)
			texDesc.Usage = D3D11_USAGE_DYNAMIC;
		else{
			if (!(access & EA_CPU_READ) && !(access & EA_CPU_WRITE))
				texDesc.Usage = D3D11_USAGE_DEFAULT;
			else
				texDesc.Usage = D3D11_USAGE_STAGING;
		}
	}

	if (access& EA_GPU_READ || (D3D11_USAGE_DYNAMIC) == texDesc.Usage)
		texDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if (access & EA_GPU_WRITE)
		texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	if (access & EA_CPU_READ)
		texDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
	if (access & EA_CPU_WRITE)
		texDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

	ID3D11Texture2D* texture = NULL;
	if (SUCCEEDED(device->CreateTexture2D(&texDesc, NULL, &texture)))
		return new D3D11Texture(texture);

	return NULL;
}

ID3D11ShaderResourceView * h3d::D3D11Factory::CreateSRV(ID3D11Texture2D * texture, const D3D11_SHADER_RESOURCE_VIEW_DESC & desc)
{
	ID3D11ShaderResourceView* srv = NULL;
	device->CreateShaderResourceView(texture, &desc, &srv);
	return srv;
}

ID3D11RenderTargetView * h3d::D3D11Factory::CreateRTV(ID3D11Texture2D * texture, const D3D11_RENDER_TARGET_VIEW_DESC * desc)
{
	ID3D11RenderTargetView* rtv = NULL;
	device->CreateRenderTargetView(texture, desc, &rtv);
	return rtv;
}

H3D_API h3d::D3D11Engine & h3d::GetEngine()
{
	static D3D11Engine mInstance;
	return mInstance;
}
