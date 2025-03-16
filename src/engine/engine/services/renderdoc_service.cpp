#include <engine/services/renderdoc_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/render_service.h>

#include <Windows.h>
#include <tlhelp32.h>
#include <ShlObj.h>

namespace engine::render
{
namespace
{

META_REGISTRATION
{
	reflection::Service<RenderDocService>("RenderDocService")
		.cli({"-rdoc"});
}


constexpr auto C_RENDER_DOC_REGISTRY_KEY = L"Applications\\qrenderdoc.exe\\shell\\open\\command";
HMODULE s_apiLibRDoc = nullptr;


bool processIsRunning(const uint32_t pid)
{
	HANDLE pss = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof(pe);

	if (Process32First(pss, &pe))
	{
		do
		{
			//-- pe.szExeFile can also be useful
			if (pe.th32ProcessID == pid)
			{
				return true;
			}
		} while (Process32Next(pss, &pe));
	}

	CloseHandle(pss);
	return false;
}


//-- Based on http://stackoverflow.com/a/1173396
void killProcess(const uint32_t pid) noexcept
{
	if (pid == 0)
	{
		return;
	}

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot)
	{
		PROCESSENTRY32 process;
		ZeroMemory(&process, sizeof(process));
		process.dwSize = sizeof(process);
		if (Process32First(snapshot, &process))
		{
			do
			{
				if (process.th32ParentProcessID == pid)
				{
					HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process.th32ProcessID);
					if (process_handle)
					{
						TerminateProcess(process_handle, 2);
						CloseHandle(process_handle);
					}
				}
			} while (Process32Next(snapshot, &process));
		}
		CloseHandle(snapshot);
	}
	HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (process_handle)
	{
		TerminateProcess(process_handle, 2);
	}
}


bool isRenderDocInstalled()
{
	DWORD type = 0;
	DWORD size = 0;
	//-- Request buffer size for a string.
	RegGetValueW(HKEY_CLASSES_ROOT, C_RENDER_DOC_REGISTRY_KEY, L"", RRF_RT_REG_SZ, &type, nullptr, &size);
	//-- Read the actual data.
	std::wstring buffer(size, L'\0');
	RegGetValueW(HKEY_CLASSES_ROOT, C_RENDER_DOC_REGISTRY_KEY, L"", RRF_RT_REG_SZ, nullptr, &buffer[0], &size);

	auto pos0 = buffer.find(L'\"');
	if (pos0 == buffer.npos)
	{
		return false;
	}
	auto pos1 = buffer.rfind(L'\\');
	if (pos1 == buffer.npos)
	{
		return false;
	}

	//-- fmt doesn't support formatting wide-strings (only conversion from utf8 to utf16).
	std::wstring path = buffer.substr(pos0 + 1, pos1 - pos0) + std::wstring(L"renderdoc.dll");
	s_apiLibRDoc = LoadLibrary(path.c_str());
	if (!s_apiLibRDoc)
	{
		logger().error("[RenderDoc]: Couldn't load renderdoc.dll");
		return false;
	}

	return true;
}

} //-- unnamed


bool RenderDocService::initialize()
{
	auto& cli = service<CLIService>().parser();
	if (!cli["-rdoc"])
	{
		return false;
	}

	if (!isRenderDocInstalled())
	{
		return false;
	}

	if (s_apiLibRDoc)
	{
		auto* getAPI = (pRENDERDOC_GetAPI)GetProcAddress(s_apiLibRDoc, "RENDERDOC_GetAPI");
		ENGINE_ASSERT(getAPI != nullptr, "Couldn't find RENDERDOC_GetAPI in the renderdoc.dll");
		[[maybe_unused]] const int ret = getAPI(eRENDERDOC_API_Version_1_6_0, (void**)&m_api);
		ENGINE_ASSERT(ret == 1, "Couldn't initialize RenderDoc API");

		m_api->SetCaptureFilePathTemplate("../../../.temp/gpu_captures/capture\0");

		m_api->SetFocusToggleKeys(NULL, 0);
		m_api->SetCaptureKeys(NULL, 0);

		//-- These options might slow down your program, enable only the ones you need.
		m_api->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_CaptureCallstacks, 1);
		m_api->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_CaptureAllCmdLists, 1);
		m_api->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_SaveAllInitials, 1);
		m_api->SetCaptureOptionU32(RENDERDOC_CaptureOption::eRENDERDOC_Option_APIValidation, 1);

		//-- Init remote access.
		m_api->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None);
	}

	return true;
}

//----------------------------------------------------------------------------------------------------------------------
void RenderDocService::release()
{
	if (processIsRunning(m_pid))
	{
		killProcess(m_pid);
	}

	if (m_api)
	{
		m_api->Shutdown();
	}

	if (s_apiLibRDoc)
	{
		FreeLibrary(s_apiLibRDoc);
		s_apiLibRDoc = nullptr;
	}
}

//----------------------------------------------------------------------------------------------------------------------
void RenderDocService::setActiveWindow(void* device, void* wnd)
{
	if (m_api)
	{
		m_api->SetActiveWindow(device, wnd);
	}
}

//----------------------------------------------------------------------------------------------------------------------
void RenderDocService::captureFrames(std::string_view /*path*/, const uint32_t numFrames)
{
	if (m_api)
	{
		m_api->TriggerMultiFrameCapture(numFrames);
		if (!processIsRunning(m_pid))
		{
			m_pid = m_api->LaunchReplayUI(1, nullptr);
		}
	}
}

} //-- engine::render.
