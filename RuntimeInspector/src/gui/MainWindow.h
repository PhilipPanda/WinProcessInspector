#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "../core/ProcessManager.h"
#include "../core/SystemInfo.h"
#include "../injection/InjectionEngine.h"

struct ImGuiContext;

namespace RuntimeInspector {
namespace GUI {

	struct ProcessInfo {
		DWORD ProcessId = 0;
		std::string ProcessName;
		std::string Architecture;
	};

	class MainWindow {
	public:
		MainWindow(HINSTANCE hInstance);
		~MainWindow();

		bool Initialize();
		int Run();

	private:
		bool InitializeDirectX();
		void CreateRenderTarget();
		void CleanupRenderTarget();
		
		void RenderUI();
		void RenderProcessList();
		void RenderDllSelection();
		void RenderInjectionMethod();
		void RenderStatusBar();
		void RefreshProcessList();
		bool BrowseForDll();
		void PerformInjection();
		HICON GetProcessIcon(DWORD processId);
		void* IconToTexture(HICON hIcon);
		void OpenProcessFileLocation(DWORD processId);
		void CopyProcessId(DWORD processId);
		void TerminateProcess(DWORD processId);
		void CopyProcessName(const std::string& processName);
		void SearchProcessOnline(const std::string& processName);

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
		bool CreateMainWindow();
		void Cleanup();

		bool InitializeImGui();
		void ShutdownImGui();
		void NewFrame();
		void Render();

		HWND m_hWnd;
		HINSTANCE m_hInstance;
		bool m_bRunning;

		ImGuiContext* m_pImGuiContext;

		std::vector<ProcessInfo> m_Processes;
		std::string m_DllPath;
		int m_SelectedProcessIndex;
		int m_SelectedInjectionMethod;
		std::string m_StatusMessage;
		std::vector<std::string> m_LogMessages;
		
		std::unordered_map<DWORD, void*> m_IconCache;

		char m_DllPathBuffer[512];
		char m_ProcessFilter[256];
		bool m_AutoRefresh;
	};

}
}
