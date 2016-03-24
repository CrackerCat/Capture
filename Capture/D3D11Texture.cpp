#include "D3D11RenderSystem.hpp"
#include "hash.hpp"

h3d::D3D11Texture::D3D11Texture(ID3D11Texture2D * tex)
	:CaptureTexture(Device_Texture),texture(tex),d3d_rtv(NULL)
{
	texture->GetDesc(&info);
}

h3d::D3D11Texture::~D3D11Texture()
{
	texture->Release();

	if (d3d_rtv)
		d3d_rtv->Release();

	srvmap_iter iter = d3d_srvs.begin();
	while (iter != d3d_srvs.end())
	{
		iter->second->Release();
		iter = d3d_srvs.erase(iter);
	}

	d3d_srvs.clear();
}

SDst h3d::D3D11Texture::GetWidth() const
{
	return info.Width;
}

SDst h3d::D3D11Texture::GetHeight() const
{
	return info.Height;
}

h3d::D3D11Texture::MappedData h3d::D3D11Texture::Map()
{
	return GetEngine().Map(this);
}

void h3d::D3D11Texture::UnMap()
{
	GetEngine().UnMap(this);
}

void h3d::D3D11Texture::Assign(D3D11Texture * src)
{
	GetEngine().CopyTexture(this, src);
}

ID3D11ShaderResourceView * h3d::D3D11Texture::RetriveD3DShaderResouceView()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Format = info.Format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = info.MipLevels ? 1 : -1;//num_levles? 由用户指定？

	//RetriveD3DShaderResouceView 可能会增加参数的额外代码 
	char const* p = reinterpret_cast<const char*>(&desc);
	size_t hash_val = h3d::hash(p, p + sizeof(desc));
	
	srvmap_iter iter = d3d_srvs.find(hash_val);
	if (iter != d3d_srvs.end())
		return iter->second;

	ID3D11ShaderResourceView* d3d_srv = GetEngine().GetFactory().CreateSRV(texture, desc);
	//in cpp11 .call emplace
	d3d_srvs[hash_val] = d3d_srv;
	return d3d_srv;
}

ID3D11RenderTargetView * h3d::D3D11Texture::RetriveD3DRenderTargetView()
{
	if(!d3d_rtv)
		d3d_rtv = GetEngine().GetFactory().CreateRTV(texture,NULL);

	//todo...

	return d3d_rtv;
}