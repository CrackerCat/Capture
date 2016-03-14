#ifndef VIDEOENGINEGDICAPTUREINJECT_H
#define VIDEOENGINEGDICAPTUREINJECT_H

#include <string>
#include "CapturePlugin.h"

namespace h3d {
	//提升权限
	bool H3D_API AdjustToken();

	//注入
	bool H3D_API InjectDLL(void * hProcess,const std::wstring& dllpath);

	//bool InjectDllBySetWindowHook
}

#endif
