#include "MainWindow.h"
#include "../core/ProcessManager.h"
#include "../injection/InjectionEngine.h"

#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_win32.h"
#include "external/imgui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <shellapi.h>
#include <psapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "psapi.lib")

using namespace RuntimeInspector::GUI;
using namespace RuntimeInspector::Core;
using namespace RuntimeInspector::Injection;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

MainWindow::MainWindow(HINSTANCE hInstance)
	: m_hWnd(nullptr)
	, m_hInstance(hInstance)
	, m_bRunning(true)
	, m_pImGuiContext(nullptr)
	, m_SelectedProcessIndex(-1)
	, m_SelectedInjectionMethod(0)
	, m_AutoRefresh(false)
{
	m_DllPathBuffer[0] = '\0';
	m_ProcessFilter[0] = '\0';
	m_StatusMessage = "Ready";
}

MainWindow::~MainWindow() {
	Cleanup();
}

bool MainWindow::Initialize() {
	if (!CreateMainWindow()) {
		return false;
	}

	if (!InitializeDirectX()) {
		return false;
	}

	if (!InitializeImGui()) {
		return false;
	}

	RefreshProcessList();
	return true;
}

bool MainWindow::CreateMainWindow() {
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = m_hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = L"RuntimeInspectorMainWindow";
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	RegisterClassExW(&wc);

	m_hWnd = ::CreateWindowExW(
		WS_EX_APPWINDOW,
		L"RuntimeInspectorMainWindow",
		L"Runtime Inspector",
		WS_POPUP | WS_SYSMENU,
		100, 100, 1000, 700,
		nullptr,
		nullptr,
		m_hInstance,
		this
	);
	
	SetWindowTextW(m_hWnd, L"Runtime Inspector");

	if (!m_hWnd) {
		return false;
	}

	SetWindowTextW(m_hWnd, L"Runtime Inspector");
	
	ShowWindow(m_hWnd, SW_SHOWNORMAL);
	UpdateWindow(m_hWnd);
	
	SetForegroundWindow(m_hWnd);
	SetActiveWindow(m_hWnd);
	SetFocus(m_hWnd);
	
	InvalidateRect(m_hWnd, nullptr, TRUE);
	UpdateWindow(m_hWnd);
	
	return true;
}

bool MainWindow::InitializeDirectX() {
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = m_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) {
		return false;
	}

	CreateRenderTarget();
	return true;
}

void MainWindow::CreateRenderTarget() {
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer) {
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void MainWindow::CleanupRenderTarget() {
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

bool MainWindow::InitializeImGui() {
	IMGUI_CHECKVERSION();
	m_pImGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(8, 6);
	style.FramePadding = ImVec2(3, 2);
	style.ItemSpacing = ImVec2(4, 3);
	style.ItemInnerSpacing = ImVec2(3, 3);
	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 8.0f;
	style.GrabRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.TabRounding = 0.0f;

	ImGui_ImplWin32_Init(m_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
	return true;
}

void MainWindow::ShutdownImGui() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

int MainWindow::Run() {
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (m_bRunning) {
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				m_bRunning = false;
			}
		}

		if (!m_bRunning) {
			break;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		RenderUI();

		ImGui::Render();
		ImVec4 bgColor = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
		const float clear_color[4] = { bgColor.x, bgColor.y, bgColor.z, bgColor.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0);
	}

	Cleanup();
	return 0;
}

void MainWindow::RenderUI() {
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Runtime Inspector", nullptr, 
		ImGuiWindowFlags_MenuBar | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	if (ImGui::BeginMenuBar()) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 1));
		ImGui::Text("RuntimeInspector");
		ImGui::PopStyleVar();
		ImGui::EndMenuBar();
	}

	float leftPanelWidth = ImGui::GetContentRegionAvail().x * 0.35f;
	ImGui::BeginChild("ProcessList", ImVec2(leftPanelWidth, 0), true);
	RenderProcessList();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("Controls", ImVec2(0, 0), true);
	RenderDllSelection();
	ImGui::Spacing();
	RenderInjectionMethod();
	ImGui::Spacing();
	
	if (ImGui::Button("Inject DLL", ImVec2(-1, 35))) {
		PerformInjection();
	}

	ImGui::Spacing();
	RenderStatusBar();
	ImGui::EndChild();

	ImGui::End();
}

void MainWindow::RenderProcessList() {
	ImGui::Text("Processes");
	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
	if (ImGui::SmallButton("Refresh")) {
		RefreshProcessList();
	}
	
	ImGui::Spacing();
	ImGui::InputText("##Filter", m_ProcessFilter, sizeof(m_ProcessFilter));
	if (m_ProcessFilter[0] == '\0') {
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetTextLineHeight() * 2);
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 0.8f), "Filter...");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::BeginChild("ProcessScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (ImGui::BeginTable("Processes", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 20);
		ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Arch", ImGuiTableColumnFlags_WidthFixed, 50);
		ImGui::TableHeadersRow();

		for (size_t i = 0; i < m_Processes.size(); i++) {
			const auto& proc = m_Processes[i];
			
			if (m_ProcessFilter[0] != '\0' && 
				proc.ProcessName.find(m_ProcessFilter) == std::string::npos) {
				continue;
			}

			ImGui::TableNextRow();
			
			ImGui::TableSetColumnIndex(0);
			void* texId = nullptr;
			
			auto it = m_IconCache.find(proc.ProcessId);
			if (it != m_IconCache.end()) {
				texId = it->second;
			}
			else {
				HICON hIcon = GetProcessIcon(proc.ProcessId);
				if (hIcon) {
					texId = IconToTexture(hIcon);
					if (texId) {
						m_IconCache[proc.ProcessId] = texId;
					}
					DestroyIcon(hIcon);
				}
			}
			
			if (texId) {
				ImGui::Image((ImTextureID)texId, ImVec2(16, 16));
			}
			else {
				ImGui::Text(" ");
			}
			
			ImGui::TableSetColumnIndex(1);
			bool selected = (m_SelectedProcessIndex == (int)i);
			ImGui::PushID((int)i);
			char pidLabel[32];
			sprintf_s(pidLabel, "%lu", proc.ProcessId);
			if (ImGui::Selectable(pidLabel, selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups)) {
				m_SelectedProcessIndex = (int)i;
			}
			
			if (ImGui::BeginPopupContextItem("ProcessContextMenu")) {
				if (ImGui::MenuItem("Open File Location")) {
					OpenProcessFileLocation(proc.ProcessId);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Copy PID")) {
					CopyProcessId(proc.ProcessId);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Terminate")) {
					TerminateProcess(proc.ProcessId);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Copy Name")) {
					CopyProcessName(proc.ProcessName);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Search Online")) {
					SearchProcessOnline(proc.ProcessName);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%s", proc.ProcessName.c_str());
			
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%s", proc.Architecture.c_str());
		}
		ImGui::EndTable();
	}

	ImGui::EndChild();
}

void MainWindow::RenderDllSelection() {
	ImGui::Text("DLL Path");
	ImGui::PushItemWidth(-80);
	ImGui::InputText("##DllPath", m_DllPathBuffer, sizeof(m_DllPathBuffer));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Browse", ImVec2(70, 0))) {
		BrowseForDll();
	}
}

void MainWindow::RenderInjectionMethod() {
	ImGui::Text("Injection Method");
	const char* methods[] = {
		"CreateRemoteThread",
		"NtCreateThreadEx",
		"QueueUserAPC",
		"SetWindowsHookEx",
		"RtlCreateUserThread"
	};
	ImGui::Combo("##Method", &m_SelectedInjectionMethod, methods, IM_ARRAYSIZE(methods));
}

void MainWindow::RenderStatusBar() {
	ImGui::Separator();
	ImGui::Text("Status: %s", m_StatusMessage.c_str());
	
	if (!m_LogMessages.empty()) {
		ImGui::Spacing();
		ImGui::Text("Log:");
		ImGui::BeginChild("Log", ImVec2(0, 120), true);
		for (const auto& msg : m_LogMessages) {
			ImGui::TextWrapped("%s", msg.c_str());
		}
		ImGui::SetScrollHereY(1.0f);
		ImGui::EndChild();
	}
}

void MainWindow::RefreshProcessList() {
	std::unordered_map<DWORD, void*> newCache;
	for (const auto& proc : m_Processes) {
		auto it = m_IconCache.find(proc.ProcessId);
		if (it != m_IconCache.end()) {
			newCache[proc.ProcessId] = it->second;
		}
	}
	
	for (auto& pair : m_IconCache) {
		if (newCache.find(pair.first) == newCache.end()) {
			if (pair.second) {
				ID3D11ShaderResourceView* pSRV = (ID3D11ShaderResourceView*)pair.second;
				if (pSRV) {
					pSRV->Release();
				}
			}
		}
	}
	m_IconCache = std::move(newCache);
	
	m_Processes.clear();
	auto processes = EnumerateAllProcesses();
	
	for (const auto& proc : processes) {
		ProcessInfo info = {};
		info.ProcessId = proc.ProcessId;
		info.ProcessName = proc.ProcessName;
		info.Architecture = proc.Architecture;
		m_Processes.push_back(info);
	}

	m_StatusMessage = "Process list refreshed (" + std::to_string(m_Processes.size()) + " processes)";
}

bool MainWindow::BrowseForDll() {
	OPENFILENAMEA ofn = {};
	char szFile[260] = { 0 };

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn)) {
		strcpy_s(m_DllPathBuffer, sizeof(m_DllPathBuffer), szFile);
		m_DllPath = szFile;
		return true;
	}
	return false;
}

void MainWindow::PerformInjection() {
	if (m_SelectedProcessIndex < 0 || m_SelectedProcessIndex >= (int)m_Processes.size()) {
		m_StatusMessage = "Error: No process selected";
		return;
	}

	if (m_DllPath.empty()) {
		m_StatusMessage = "Error: No DLL path specified";
		return;
	}

	DWORD pid = m_Processes[m_SelectedProcessIndex].ProcessId;
	HANDLE hProcess = OpenTargetProcess(pid);
	if (!hProcess) {
		m_StatusMessage = "Error: Failed to open target process";
		return;
	}

	bool success = false;
	switch (m_SelectedInjectionMethod) {
		case 0:
			success = InjectViaCreateRemoteThread(m_DllPath.c_str(), hProcess);
			break;
		case 1:
			success = InjectViaNtCreateThreadEx(m_DllPath.c_str(), hProcess);
			break;
		case 2:
			success = InjectViaQueueUserAPC(m_DllPath.c_str(), hProcess, pid);
			break;
		case 3:
			success = InjectViaSetWindowsHookEx(pid, m_DllPath.c_str());
			break;
		case 4:
			success = InjectViaRtlCreateUserThread(hProcess, m_DllPath.c_str());
			break;
	}

	CloseHandle(hProcess);

	if (success) {
		m_StatusMessage = "Injection successful!";
		m_LogMessages.push_back("Successfully injected " + m_DllPath + " into PID " + std::to_string(pid));
	}
	else {
		m_StatusMessage = "Injection failed!";
		m_LogMessages.push_back("Failed to inject " + m_DllPath + " into PID " + std::to_string(pid));
	}
}

void MainWindow::Cleanup() {
	for (auto& pair : m_IconCache) {
		if (pair.second) {
			ID3D11ShaderResourceView* pSRV = (ID3D11ShaderResourceView*)pair.second;
			if (pSRV) {
				pSRV->Release();
			}
		}
	}
	m_IconCache.clear();
	
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

	ShutdownImGui();

	char imguiIniPath[MAX_PATH];
	if (GetModuleFileNameA(nullptr, imguiIniPath, MAX_PATH)) {
		char* lastSlash = strrchr(imguiIniPath, '\\');
		if (lastSlash) {
			*(lastSlash + 1) = '\0';
			strcat_s(imguiIniPath, MAX_PATH, "imgui.ini");
			DeleteFileA(imguiIniPath);
		}
	}

	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if ((uMsg < WM_NCMOUSEMOVE || uMsg > WM_NCMBUTTONDBLCLK) && 
		uMsg != WM_NCHITTEST &&
		ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return true;

	if (uMsg == WM_NCCREATE) {
		CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
		MainWindow* pThis = (MainWindow*)pCreate->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	MainWindow* pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (pThis) {
		return pThis->HandleMessage(uMsg, wParam, lParam);
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_ACTIVATE:
			if (wParam != WA_INACTIVE) {
				SetForegroundWindow(m_hWnd);
				InvalidateRect(m_hWnd, nullptr, TRUE);
			}
			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		case WM_NCACTIVATE:
			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(m_hWnd, &ps);
			EndPaint(m_hWnd, &ps);
			return 0;
		}
		case WM_SIZE:
			if (g_pSwapChain && wParam != SIZE_MINIMIZED) {
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU)
				return 0;
			if ((wParam & 0xfff0) == SC_CLOSE) {
				m_bRunning = false;
				PostQuitMessage(0);
				return 0;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			m_bRunning = false;
			return 0;
		case WM_NCHITTEST:
			LRESULT hit = DefWindowProc(m_hWnd, uMsg, wParam, lParam);
			if (hit == HTCLIENT) {
				POINT pt = { LOWORD(lParam), HIWORD(lParam) };
				ScreenToClient(m_hWnd, &pt);
				if (pt.y >= 0 && pt.y < 25) {
					return HTCAPTION;
				}
			}
			return hit;
	}
	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

HICON MainWindow::GetProcessIcon(DWORD processId) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (!hProcess) {
		return nullptr;
	}

	char exePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameExA(hProcess, nullptr, exePath, MAX_PATH) == 0) {
		CloseHandle(hProcess);
		return nullptr;
	}

	CloseHandle(hProcess);

	SHFILEINFOA sfi = { 0 };
	SHGetFileInfoA(exePath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
	return sfi.hIcon;
}

void* MainWindow::IconToTexture(HICON hIcon) {
	if (!hIcon || !g_pd3dDevice) {
		return nullptr;
	}

	ICONINFO iconInfo = { 0 };
	if (!GetIconInfo(hIcon, &iconInfo)) {
		return nullptr;
	}

	BITMAP bmp = { 0 };
	if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp)) {
		if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
		if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
		return nullptr;
	}

	int width = bmp.bmWidth;
	int height = bmp.bmHeight;

	HDC hDC = CreateCompatibleDC(nullptr);
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	unsigned int* pixels = nullptr;
	HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, (void**)&pixels, nullptr, 0);
	if (!hBitmap) {
		DeleteDC(hDC);
		return nullptr;
	}

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDC, hBitmap);
	DrawIconEx(hDC, 0, 0, hIcon, width, height, 0, nullptr, DI_NORMAL);
	SelectObject(hDC, hOldBitmap);
	DeleteDC(hDC);

	for (int i = 0; i < width * height; i++) {
		unsigned int pixel = pixels[i];
		unsigned char b = (pixel >> 0) & 0xFF;
		unsigned char g = (pixel >> 8) & 0xFF;
		unsigned char r = (pixel >> 16) & 0xFF;
		unsigned char a = (pixel >> 24) & 0xFF;
		pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
	}

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = pixels;
	subResource.SysMemPitch = width * 4;
	subResource.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture = nullptr;
	if (FAILED(g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture))) {
		DeleteObject(hBitmap);
		if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
		if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { 0 };
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* pSRV = nullptr;
	if (FAILED(g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV))) {
		pTexture->Release();
		DeleteObject(hBitmap);
		if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
		if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
		return nullptr;
	}

	pTexture->Release();
	DeleteObject(hBitmap);
	if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
	if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

	return (void*)pSRV;
}

void MainWindow::OpenProcessFileLocation(DWORD processId) {
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (!hProcess) {
		m_StatusMessage = "Failed to open process";
		return;
	}

	char exePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameExA(hProcess, nullptr, exePath, MAX_PATH) == 0) {
		CloseHandle(hProcess);
		m_StatusMessage = "Failed to get process path";
		return;
	}

	CloseHandle(hProcess);

	char* lastSlash = strrchr(exePath, '\\');
	if (lastSlash) {
		*lastSlash = '\0';
	}

	std::string folderPath = exePath;
	ShellExecuteA(nullptr, "open", folderPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	m_StatusMessage = "Opened file location";
}

void MainWindow::CopyProcessId(DWORD processId) {
	std::string pidStr = std::to_string(processId);
	
	if (OpenClipboard(nullptr)) {
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, pidStr.length() + 1);
		if (hMem) {
			memcpy(GlobalLock(hMem), pidStr.c_str(), pidStr.length() + 1);
			GlobalUnlock(hMem);
			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
			m_StatusMessage = "PID copied to clipboard";
		}
		else {
			CloseClipboard();
			m_StatusMessage = "Failed to copy PID";
		}
	}
	else {
		m_StatusMessage = "Failed to open clipboard";
	}
}

void MainWindow::TerminateProcess(DWORD processId) {
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
	if (!hProcess) {
		m_StatusMessage = "Failed to open process for termination";
		return;
	}

	if (::TerminateProcess(hProcess, 0)) {
		m_StatusMessage = "Process terminated";
		m_LogMessages.push_back("Terminated process PID " + std::to_string(processId));
		RefreshProcessList();
	}
	else {
		m_StatusMessage = "Failed to terminate process";
	}

	CloseHandle(hProcess);
}

void MainWindow::CopyProcessName(const std::string& processName) {
	if (OpenClipboard(nullptr)) {
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, processName.length() + 1);
		if (hMem) {
			memcpy(GlobalLock(hMem), processName.c_str(), processName.length() + 1);
			GlobalUnlock(hMem);
			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
			m_StatusMessage = "Process name copied to clipboard";
		}
		else {
			CloseClipboard();
			m_StatusMessage = "Failed to copy process name";
		}
	}
	else {
		m_StatusMessage = "Failed to open clipboard";
	}
}

void MainWindow::SearchProcessOnline(const std::string& processName) {
	std::string searchUrl = "https://www.google.com/search?q=" + processName;
	ShellExecuteA(nullptr, "open", searchUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	m_StatusMessage = "Searching online for " + processName;
}
