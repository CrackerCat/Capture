#include "SceneCapture.h"

using namespace h3d;


#include <list>
#include "D3D11RenderSystem.hpp"
#include "CPURenderSystem.h"
#include "WinPlatform.h"
#ifdef min
#undef min
#endif
#include<algorithm>

class PartialSceneCapture : public SceneCapture {
public:
	PartialSceneCapture(const char * path);

	virtual ~PartialSceneCapture();

	virtual bool AddCapture(const char* name, CaptureType type, unsigned long native);

	bool RemoveCapture(const char * name) override;

	bool Deserialize(const char* path) override;

	void Stop() override;
protected:
	struct CaptrueItem
	{
		std::string name;
		CaptureHUB* capture;

		CaptureTexture* middle_tex;
	};
	std::list<CaptrueItem> items;
	typedef std::list<CaptrueItem>::iterator item_iterator;
};


class D3D11SceneCapture : public PartialSceneCapture {
public:
	D3D11SceneCapture(const char * path);

	~D3D11SceneCapture();

	bool AddCapture(const char* name, CaptureType type, unsigned long native) override;

	CaptureTexture* Capture() override;
private:
	D3D11Texture* capture_rtv;
	D3D11Texture* capture_tex;
};

class CPUSceneCapture : public PartialSceneCapture {
public:
	CPUSceneCapture(const char * path);
	~CPUSceneCapture();
	CaptureTexture* Capture() override;
private:
	MemoryTexture* capture_rtv;
};


D3D11SceneCapture::D3D11SceneCapture(const char * path)
	:PartialSceneCapture(path),capture_tex(NULL),capture_rtv(NULL)
{
	capture_rtv = GetEngine().GetFactory().CreateTexture(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), BGRA8, EA_GPU_WRITE);
	capture_tex = GetEngine().GetFactory().CreateTexture(capture_rtv->GetWidth(),capture_rtv->GetHeight(), BGRA8,EA_CPU_READ);
}

D3D11SceneCapture::~D3D11SceneCapture()
{
	delete capture_tex;
	delete capture_rtv;
}

bool D3D11SceneCapture::AddCapture(const char* name, CaptureType type, unsigned long native) {
	if (PartialSceneCapture::AddCapture(name, type, native)) {
		CaptureHUB* capture = items.end()->capture;
		CaptureTexture* texture = capture->Capture();
		//warning! 1920*1080 * >= 30 FPS 可能会造成显卡带宽瓶颈
		if (texture && texture->GetType() == CaptureTexture::Memory_Texture)
			items.end()->middle_tex = GetEngine().GetFactory().CreateTexture(texture->GetWidth(), texture->GetHeight(), BGRA8, EA_CPU_WRITE | EA_GPU_READ);

		return true;
	}
	return false;
}

CaptureTexture * D3D11SceneCapture::Capture()
{
	item_iterator iter = items.begin();
	//注意，先填充填图
	for (; iter != items.end(); ++iter) {
		D3D11Texture* middle_tex = static_cast<D3D11Texture*>(iter->middle_tex);
		if (middle_tex) {
			CaptureTexture* texture = iter->capture->Capture();
			CaptureTexture::MappedData dev = middle_tex->Map();
			CaptureTexture::MappedData mem = texture->Map();

			if (dev.RowPitch == mem.RowPitch)
				memcpy(dev.pData, mem.pData, mem.RowPitch*texture->GetHeight());
			else
				for (unsigned y = 0; y != texture->GetHeight(); ++y)
					memcpy(dev.pData + dev.RowPitch*y, mem.pData + mem.RowPitch*y, std::min(mem.RowPitch, dev.RowPitch));

			middle_tex->UnMap();
			texture->UnMap();
		}
	}

	GetEngine().BeginDraw(capture_rtv,OVERLAY_DRAW);

	iter = items.begin();
	for (; iter != items.end(); ++iter) {
		D3D11Texture* texture = NULL;
		D3D11Texture* middle_tex = reinterpret_cast<D3D11Texture*>(iter->middle_tex);
		if (middle_tex)
			texture = middle_tex;
		else
			texture = static_cast<D3D11Texture*>(iter->capture->Capture());

		if(texture)
			GetEngine().Draw(0,0, texture->GetWidth(), texture->GetHeight(), texture,iter->capture->Flip());
	}

	GetEngine().EndDraw();
	capture_tex->Assign(capture_rtv);
	return capture_tex;
}

CPUSceneCapture::CPUSceneCapture(const char * path)
	:PartialSceneCapture(path)
{
	capture_rtv = GetCPUEngine().GetFactory().CreateTexture(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

CPUSceneCapture::~CPUSceneCapture()
{
	delete capture_rtv;
}

CaptureTexture* CPUSceneCapture::Capture() {
	GetCPUEngine().BeginDraw(capture_rtv, OVERLAY_DRAW);
	item_iterator iter = items.begin();
	for (; iter != items.end(); ++iter) {
		CaptureTexture* texture = iter->capture->Capture();
		GetCPUEngine().Draw(0, 0, texture->GetWidth(), texture->GetHeight(), texture, iter->capture->Flip());
	}
	GetEngine().EndDraw();
	return capture_rtv;
}

SceneCapture * h3d::SceneCapture::Serialize(const char * path)
{
#ifdef _USING_V110_SDK71_
	return new CPUSceneCapture(path);
#endif
	if (h3d::GetOSVersion() >= 7 && GetEngine().GetLevel() >= D3D_FEATURE_LEVEL_9_1)
		return new D3D11SceneCapture(path);
	else
		return new CPUSceneCapture(path);
}

//#include "tinyxml/tinyxml.h"
#include "CameraCapture.h"
#include "GDICapture.h"
#include "GraphicCapture.h"
PartialSceneCapture::PartialSceneCapture(const char * path)
{
}

PartialSceneCapture::~PartialSceneCapture()
{
	item_iterator iter = items.begin();
	while (iter != items.end()) {
		delete iter->capture;
		delete iter->middle_tex;
		iter = items.erase(iter);
	}
}

bool PartialSceneCapture::AddCapture(const char * name, CaptureType type, unsigned long native)
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
	item.middle_tex = NULL;

	items.push_back(item);

	return true;
}

bool PartialSceneCapture::RemoveCapture(const char * name)
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

bool PartialSceneCapture::Deserialize(const char * path)
{
	return false;
}

void PartialSceneCapture::Stop()
{
	item_iterator iter = items.begin();
	for (; iter != items.end(); ++iter)
		iter->capture->Stop();
}
