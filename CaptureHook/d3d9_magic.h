#ifndef D3D9_MAGIC_SUPPORT
#define D3D9_MAGIC_SUPPORT




namespace h3d {
	bool D3D9PatchTypeSupportSharedTexture(void* module_address);

	//���������Դ��OBS�����о� �����������ǰ�ȵ���D3D9PatchTypeSupportSharedTexture
	//D3D9PatchTypeSupportSharedTexture ������EndScene����SharedTextureHandle����
	void BeginD3D9Patch(void* module_address);
	void EndD3D9Patch(void* module_address);
}

#endif
