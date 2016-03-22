#ifndef VIDEOENGINECAPTURESCENE_H
#define VIDEOENGINECAPTURESCENE_H

#include "CaptureHUB.h"
namespace h3d {
	enum CaptureType {
		CAMERA_CAPTURE,
		WINDOW_CAPTURE,
		GAME_CAPTURE
	};


	class H3D_API SceneCapture : public CaptureHUB {
	public:
		SceneCapture() {}

		virtual ~SceneCapture() {}

	public:
		//type = CAMERA_CAPTURE native should be device index
		//type = WINDOW_CAPTURE native should be windows's hwnd
		//type = GAME_CAPTURE native should be game processId
		virtual bool AddCapture(const char* name,CaptureType type, unsigned long native) = 0;

		virtual bool RemoveCapture(const char * name) = 0;

		virtual	bool Deserialize(const char* path) = 0;

	public:
		static SceneCapture*  Serialize(const char* path);
	};

}


#endif
