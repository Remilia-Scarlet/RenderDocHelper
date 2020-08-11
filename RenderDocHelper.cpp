#include "RenderDocHelper.h"

#include <string>
#include <Windows.h>
#include <winreg.h>
#include <filesystem>

#include "renderdoc_app.h"


bool QueryRegKey(const HKEY InKey, const TCHAR* InSubKey, const TCHAR* InValueName, std::wstring& OutData)
{
	bool bSuccess = false;

	// Redirect key depending on system
	for (int RegistryIndex = 0; RegistryIndex < 2 && !bSuccess; ++RegistryIndex)
	{
		HKEY Key = 0;
		const uint32_t RegFlags = (RegistryIndex == 0) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
		if (RegOpenKeyEx(InKey, InSubKey, 0, KEY_READ | RegFlags, &Key) == ERROR_SUCCESS)
		{
			::DWORD Size = 0;
			// First, we'll call RegQueryValueEx to find out how large of a buffer we need
			if ((RegQueryValueEx(Key, InValueName, NULL, NULL, NULL, &Size) == ERROR_SUCCESS) && Size)
			{
				// Allocate a buffer to hold the value and call the function again to get the data
				char *Buffer = new char[Size];
				if (RegQueryValueEx(Key, InValueName, NULL, NULL, (LPBYTE)Buffer, &Size) == ERROR_SUCCESS)
				{
					const size_t Length = (Size / sizeof(TCHAR)) - 1;
					OutData = std::wstring((wchar_t*)Buffer, Length);
					bSuccess = true;
				}
				delete[] Buffer;
			}
			RegCloseKey(Key);
		}
	}

	return bSuccess;
}


RenderDocHelper * RenderDocHelper::instance()
{
	static RenderDocHelper* ret = nullptr;
	if(!ret)
	{
		static RenderDocHelper instance;
		ret = &instance;
		ret->initRenderDoc();
	}
	return ret;
}

RenderDocHelper::~RenderDocHelper()
{
	if(RenderDocDLL)
		::FreeLibrary((HMODULE)RenderDocDLL);
}

bool RenderDocHelper::initRenderDoc()
{
	RENDERDOC_API_1_4_1* RenderDocAPI = nullptr;

	std::wstring QueryPath;
	QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\"), TEXT(""), QueryPath);
	std::experimental::filesystem::path RenderdocPath(QueryPath);
	RenderdocPath = RenderdocPath.replace_filename("renderdoc.dll");

	RenderDocDLL = LoadLibrary(RenderdocPath.c_str());
	if (RenderDocDLL == nullptr)
	{
		return false;
	}

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)(void*)::GetProcAddress((HMODULE)RenderDocDLL, "RENDERDOC_GetAPI");
	if (RENDERDOC_GetAPI == nullptr)
	{
		::FreeLibrary((HMODULE)RenderDocDLL);
		return false;
	}

	// Version checking and reporting
	if (0 == RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&RenderDocAPI))
	{
		::FreeLibrary((HMODULE)RenderDocDLL);
		return false;
	}

	int MajorVersion(0), MinorVersion(0), PatchVersion(0);
	RenderDocAPI->GetAPIVersion(&MajorVersion, &MinorVersion, &PatchVersion);
	if (MajorVersion == 0)
	{
		::FreeLibrary((HMODULE)RenderDocDLL);
		return false;
	}
	RenderDocAPI->SetLogFilePathTemplate("RenderdocCapture\\");
	
	RenderDocAPI->SetFocusToggleKeys(nullptr, 0);
	
	RENDERDOC_InputButton prtScrKey = eRENDERDOC_Key_PrtScrn;
	RenderDocAPI->SetCaptureKeys(&prtScrKey, 1);

	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 0);
	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 0);
	RenderDocAPI->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

	//RenderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
	
	return true;
}
