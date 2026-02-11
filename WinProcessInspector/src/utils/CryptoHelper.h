#pragma once

#include <Windows.h>
#include <string>

namespace WinProcessInspector {
namespace Utils {

	class CryptoHelper {
	public:
		static std::wstring CalculateMD5(const std::wstring& filePath);
		static std::wstring CalculateSHA1(const std::wstring& filePath);
		static std::wstring CalculateSHA256(const std::wstring& filePath);
		
	private:
		static std::wstring CalculateHash(const std::wstring& filePath, DWORD algId);
		static std::wstring BytesToHexString(const BYTE* bytes, DWORD length);
	};

} // namespace Utils
} // namespace WinProcessInspector
