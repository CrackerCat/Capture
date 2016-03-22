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
};


SceneCapture * h3d::SceneCapture::Serialize(const char * path)
{
	//todo support other blend
	return new D3D11SceneCapture(path);
}


D3D11SceneCapture::D3D11SceneCapture(const char * path)
{
}

D3D11SceneCapture::~D3D11SceneCapture()
{
	item_iterator iter = items.begin();
	while(iter != items.end()) {
		delete iter->capture;
		delete iter->middle_tex;
		iter = items.erase(iter);
	}
}

#ifdef min
#undef min
#endif
#include<algorithm>

CaptureTexture * D3D11SceneCapture::Capture()
{
	item_iterator iter = items.begin();
	for (; iter != items.end(); ++iter) {
		CaptureTexture* texture = iter->capture->Capture();
		if (texture->GetType() == Memory_Texture) {
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


	return nullptr;
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
	if (texture->GetType() == Memory_Texture)
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
