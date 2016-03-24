#include "SceneCapture.h"

using namespace h3d;

//#include "tinyxml/tinyxml.h"

#include <list>
#include "D3D11RenderSystem.hpp"

class D3D11SceneCapture : public SceneCapture {
public:
	D3D11SceneCapture(const char * path);

	~D3D11SceneCapture();

	CaptureTexture* Capture();

	void Stop();
public:
	bool AddCapture(const char* name, CaptureType type, unsigned long native);

	bool RemoveCapture(const char * name);

	bool Deserialize(const char* path);
private:
	struct CaptrueItem
	{
		std::string name;
		CaptureHUB* capture;

		D3D11Texture* middle_tex;

	};
	std::list<CaptrueItem> items;
	typedef std::list<CaptrueItem>::iterator item_iterator;

	D3D11Texture* capture_rtv;
	D3D11Texture* capture_tex;
};


SceneCapture * h3d::SceneCapture::Serialize(const char * path)
{
	//todo support other blend
	return new D3D11SceneCapture(path);
}


D3D11SceneCapture::D3D11SceneCapture(const char * path)
	:capture_tex(NULL),capture_rtv(NULL)
{
	//从xml里面载入相关信息，暂时都假定
	capture_rtv = GetEngine().GetFactory().CreateTexture(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), BGRA8, EA_GPU_WRITE);
	capture_tex = GetEngine().GetFactory().CreateTexture(capture_rtv->GetWidth(),capture_rtv->GetHeight(), BGRA8,EA_CPU_READ);
}

D3D11SceneCapture::~D3D11SceneCapture()
{
	item_iterator iter = items.begin();
	while(iter != items.end()) {
		delete iter->capture;
		delete iter->middle_tex;
		iter = items.erase(iter);
	}

	delete capture_tex;
	delete capture_rtv;
}

#ifdef min
#undef min
#endif
#include<algorithm>

CaptureTexture * D3D11SceneCapture::Capture()
{
	item_iterator iter = items.begin();
	//注意，先填充填图
	for (; iter != items.end(); ++iter) {
		if (iter->middle_tex) {
			CaptureTexture* texture = iter->capture->Capture();
			CaptureTexture::MappedData dev = iter->middle_tex->Map();
			CaptureTexture::MappedData mem = texture->Map();

			if (dev.RowPitch == mem.RowPitch)
				memcpy(dev.pData, mem.pData, mem.RowPitch*texture->GetHeight());
			else
				for (unsigned y = 0; y != texture->GetHeight(); ++y)
					memcpy(dev.pData + dev.RowPitch*y, mem.pData + mem.RowPitch*y, std::min(mem.RowPitch, dev.RowPitch));

			iter->middle_tex->UnMap();
			texture->UnMap();
		}
	}

	GetEngine().BeginDraw(capture_rtv,OVERLAY_DRAW);

	iter = items.begin();
	for (; iter != items.end(); ++iter) {
		D3D11Texture* texture = NULL;
		if (iter->middle_tex)
			texture = iter->middle_tex;
		else
			texture = static_cast<D3D11Texture*>(iter->capture->Capture());

		GetEngine().Draw(0,0, texture->GetWidth(), texture->GetHeight(), texture,iter->capture->Flip());
	}

	GetEngine().EndDraw();
	capture_tex->Assign(capture_rtv);
	return capture_tex;
}

void D3D11SceneCapture::Stop()
{
	item_iterator iter = items.begin();
	for (; iter != items.end(); ++iter)
		iter->capture->Stop();
}

#include "CameraCapture.h"
#include "GDICapture.h"
#include "GraphicCapture.h"
bool D3D11SceneCapture::AddCapture(const char * name, CaptureType type, unsigned long native)
{
	CaptureHUB* capture = NULL;
	switch (type)
	{
	case h3d::CAMERA_CAPTURE:
		capture = new CameraCapture(CaptureInfo(),static_cast<unsigned int>(native));
		break;
	case h3d::WINDOW_CAPTURE:
		capture = new GDICapture(CaptureInfo(reinterpret_cast<void*>(native)));
		break;
	case h3d::GAME_CAPTURE:
		capture = GraphicCapture(native);
		break;
	default:
		return false;
	}
	if (!capture)
		return false;

	CaptrueItem item;
	item.capture = capture;
	item.name = name;

	CaptureTexture* texture = capture->Capture();
	//warning! 1920*1080 * >= 30 FPS 可能会造成显卡带宽瓶颈
	if (texture->GetType() == CaptureTexture::Memory_Texture)
		item.middle_tex = GetEngine().GetFactory().CreateTexture(texture->GetWidth(), texture->GetHeight(), BGRA8, EA_CPU_WRITE | EA_GPU_READ);
	else
		item.middle_tex = NULL;
	
	items.push_back(item);

	return true;
}

bool D3D11SceneCapture::RemoveCapture(const char * name)
{
	item_iterator iter = items.begin();
	for (; iter != items.end(); ++iter)
		if (iter->name == name)
			break;
	if (iter != items.end()) {
		delete iter->capture;
		delete iter->middle_tex;
		items.erase(iter);
		return true;
	}

	return true;
}

bool D3D11SceneCapture::Deserialize(const char * path)
{
	return false;
}
