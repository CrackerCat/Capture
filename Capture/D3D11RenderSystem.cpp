#include "D3D11RenderSystem.hpp"
#include "WinPlatform.h"
#include <stdexcept>
#include <fstream>

#ifndef _USING_V110_SDK71_
#include <dxgi1_2.h>
#endif

static const char* FormatString(DXGI_FORMAT format);


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
		{
			std::vector<char> buffer = ReadShaderFile(L"Shader/DrawVS.cso");
			D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			factory->device->CreateVertexShader(&buffer[0], buffer.size(), NULL, &draw_vs);
			factory->device->CreateInputLayout(layout_desc,2, &buffer[0], buffer.size(), &draw_il);

			buffer.swap(ReadShaderFile(L"Shader/ReslovePS.cso"));
			factory->device->CreatePixelShader(&buffer[0], buffer.size(), NULL, &draw_ps);
		}

		float vertices[4 * 4] = {
			1,1,1,1,//1,0
			1,-1,1,1, //1,1
			-1,1,1,1,//0,0
			-1,-1,1,1//0,1
		};
		memcpy(&draw_vertexs[0],&vertices[0],16);
		memcpy(&draw_vertexs[1], &vertices[4], 16);
		memcpy(&draw_vertexs[2], &vertices[8], 16);
		memcpy(&draw_vertexs[3], &vertices[12], 16);
		draw_vertexs[0].u = draw_vertexs[1].v = draw_vertexs[1].u = draw_vertexs[3].v = 1;
		draw_vertexs[0].v = draw_vertexs[2].v = draw_vertexs[2].u = draw_vertexs[3].u = 0;

		draw_stride_offset[0] = sizeof(vertex);
		draw_stride_offset[1] = 0;

		CD3D11_BUFFER_DESC vbDesc(sizeof(draw_vertexs), D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE);
		D3D11_SUBRESOURCE_DATA resDesc;
		resDesc.pSysMem = draw_vertexs;
		factory->device->CreateBuffer(&vbDesc, &resDesc, &draw_vb);

		CD3D11_SAMPLER_DESC point_sampler(D3D11_DEFAULT);
		point_sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		factory->device->CreateSamplerState(&point_sampler, &draw_ps_ss);
	}

	back_buffer->Release();
	pAdapter->Release();
	pFactory->Release();

	return true;
}

#define SR(var) if(var) {var->Release();var = NULL;}

void h3d::D3D11Engine::Destroy()
{
	SR(draw_il);
	SR(draw_ps);
	SR(draw_ps_ss);
	SR(draw_vb);
	SR(draw_vs);

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

void h3d::D3D11Engine::BeginDraw(D3D11Texture * rt, BLEND_TYPE bt)
{
	ID3D11RenderTargetView* rtv = rt->RetriveD3DRenderTargetView();
	context->OMSetRenderTargets(1, &rtv, NULL);
	const float rgba[] = {0,0,0,0};
	context->ClearRenderTargetView(rtv, rgba);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vp.TopLeftY = 0;
	vp.Width = static_cast<FLOAT>(rt->GetWidth());
	vp.Height = static_cast<FLOAT>(rt->GetHeight());
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);
	curr_vp = vp;

	context->IASetVertexBuffers(0, 1, &draw_vb,draw_stride_offset, &draw_stride_offset[1]);
	context->IASetInputLayout(draw_il);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->VSSetShader(draw_vs, NULL, 0);
	context->PSSetShader(draw_ps, NULL, 0);
	context->PSSetSamplers(0, 1, &draw_ps_ss);
}

#include <fstream>
extern std::ofstream logstream;
void h3d::D3D11Engine::Draw(SDst x, SDst y, SDst width, SDst height, D3D11Texture * src,bool flip)
{
	//Update VB;
	float ndc_left_top[2] = { x / curr_vp.Width * 2.f - 1,1 - y / curr_vp.Height * 2.f};
	float ndc_right_bottom[2] = { (x+width)/curr_vp.Width * 2.f - 1,1 - (y+height)/curr_vp.Height*2.f};
	draw_vertexs[0].x = ndc_right_bottom[0]; draw_vertexs[0].y = ndc_left_top[1];
	draw_vertexs[1].x = ndc_right_bottom[0]; draw_vertexs[1].y = ndc_right_bottom[1];
	draw_vertexs[2].x = ndc_left_top[0]; draw_vertexs[2].y = ndc_left_top[1];
	draw_vertexs[3].x = ndc_left_top[0]; draw_vertexs[3].y = ndc_right_bottom[1];

	if (flip)
		draw_vertexs[0].v = draw_vertexs[2].v = 1.f, draw_vertexs[1].v = draw_vertexs[3].v = 0.f;
	else
		draw_vertexs[0].v = draw_vertexs[2].v = 0.f, draw_vertexs[1].v = draw_vertexs[3].v = 1.f;

	D3D11_MAPPED_SUBRESOURCE subRes;
	context->Map(draw_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);
	memcpy(subRes.pData, draw_vertexs, sizeof(draw_vertexs));
	context->Unmap(draw_vb, 0);

	ID3D11ShaderResourceView* srv = src->RetriveD3DShaderResouceView();
	context->PSSetShaderResources(0, 1, &srv);
	context->Draw(4, 0);
}

void h3d::D3D11Engine::EndDraw()
{
	context->ClearState();
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

h3d::D3D11Texture * h3d::D3D11Factory::CreateGDITexture(SDst Width, SDst Height)
{
	CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_B8G8R8A8_UNORM, Width, Height, 1, 1);
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	ID3D11Texture2D* texture = NULL;
	if (SUCCEEDED(device->CreateTexture2D(&texDesc, NULL, &texture))) {
		logstream << "CreateGDITexture Format : " << FormatString(DXGI_FORMAT_B8G8R8A8_UNORM) << " Size : " << Width << 'X'<<Height<<std::endl;
		return new D3D11GDITexture(texture);
	}

	return NULL;
}

h3d::D3D11Texture * h3d::D3D11Factory::CreateTexture(unsigned __int64 Handle)
{
	HANDLE hShare = reinterpret_cast<HANDLE>(Handle);

	ID3D11Resource* share_res = NULL;

	if (SUCCEEDED(device->OpenSharedResource(hShare, __uuidof(ID3D11Resource), reinterpret_cast<void**>(&share_res)))) {
		ID3D11Texture2D* share_tex = NULL;
		if (SUCCEEDED(share_res->QueryInterface(&share_tex))) {
			share_res->Release();
			D3D11_TEXTURE2D_DESC desc;
			share_tex->GetDesc(&desc);
			logstream << "CreateSharedTexture Format : " << FormatString(desc.Format) << " Size : " << desc.Width << 'X' << desc.Height << std::endl;
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
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case h3d::BGRX8:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case h3d::B5G6R5A1: case h3d::B5G6R5X1:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case h3d::B5G6R5:
		return DXGI_FORMAT_B5G6R5_UNORM;
	case h3d::RGBA8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case h3d::RGBA16:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case h3d::R10G10B10XRA2:
		return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
	}
	return DXGI_FORMAT_UNKNOWN;
}

static const char* FormatString(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		return "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return "DXGI_FORMAT_R16G16B16A16_FLOAT";
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return "DXGI_FORMAT_R8G8B8A8_UNORM";
	case DXGI_FORMAT_B5G6R5_UNORM:
		return "DXGI_FORMAT_B5G6R5_UNORM";
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return "DXGI_FORMAT_B5G5R5A1_UNORM";
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return "DXGI_FORMAT_B8G8R8X8_UNORM";
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return "DXGI_FORMAT_B8G8R8A8_UNORM";
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return "DXGI_FORMAT_R10G10B10A2_UNORM";
	}
	return "DXGI_FORMAT_UNKNOWN";
}

#pragma warning(disable:4244)
h3d::D3D11Texture * h3d::D3D11Factory::CreateTexture(SDst Width, SDst Height, SWAPFORMAT Format,unsigned int access)
{
	CD3D11_TEXTURE2D_DESC texDesc(ConvertFormat(Format),Width, Height, 1, 1);

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

	texDesc.BindFlags = 0;
	if (access& EA_GPU_READ || (D3D11_USAGE_DYNAMIC) == texDesc.Usage)
		texDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	if (access & EA_GPU_WRITE)
		texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

	if (access & EA_CPU_READ)
		texDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
	if (access & EA_CPU_WRITE)
		texDesc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

	ID3D11Texture2D* texture = NULL;
	if (SUCCEEDED(device->CreateTexture2D(&texDesc, NULL, &texture))) {
		logstream << "CreateTexture Format : " << FormatString(texDesc.Format) << " Size : " << Width << 'X' << Height << std::endl;
		return new D3D11Texture(texture);
	}

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


