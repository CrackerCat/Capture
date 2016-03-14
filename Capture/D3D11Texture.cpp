#include "D3D11RenderSystem.hpp"

h3d::D3D11Texture::D3D11Texture(ID3D11Texture2D * tex)
	:texture(tex)
{
	texture->GetDesc(&info);
}

h3d::D3D11Texture::~D3D11Texture()
{
	texture->Release();
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