#ifndef VIDEOENGINEGDICAPTUREINJECT_H
#define VIDEOENGINEGDICAPTUREINJECT_H

#include <string>


namespace h3d {
	//����Ȩ��
	bool AdjustToken();

	//ע��
	bool  InjectDLL(void * hProcess,const std::wstring& dllpath);
}

#endif
