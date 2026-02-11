#include "CryptoHelper.h"
#include <wincrypt.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>

#pragma comment(lib, "advapi32.lib")

namespace WinProcessInspector {
namespace Utils {

std::wstring CryptoHelper::CalculateMD5(const std::wstring& filePath) {
	return CalculateHash(filePath, CALG_MD5);
}

std::wstring CryptoHelper::CalculateSHA1(const std::wstring& filePath) {
	return CalculateHash(filePath, CALG_SHA1);
}

std::wstring CryptoHelper::CalculateSHA256(const std::wstring& filePath) {
	return CalculateHash(filePath, CALG_SHA_256);
}

std::wstring CryptoHelper::CalculateHash(const std::wstring& filePath, DWORD algId) {
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		return L"";
	}

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	
	if (!CryptAcquireContextW(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		return L"";
	}

	if (!CryptCreateHash(hProv, algId, 0, 0, &hHash)) {
		CryptReleaseContext(hProv, 0);
		return L"";
	}

	const size_t BUFFER_SIZE = 8192;
	std::vector<BYTE> buffer(BUFFER_SIZE);
	
	while (file.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE) || file.gcount() > 0) {
		DWORD bytesRead = static_cast<DWORD>(file.gcount());
		if (!CryptHashData(hHash, buffer.data(), bytesRead, 0)) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			return L"";
		}
	}

	file.close();

	DWORD hashLen = 0;
	DWORD hashLenSize = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashLen), &hashLenSize, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return L"";
	}

	std::vector<BYTE> hashBytes(hashLen);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, hashBytes.data(), &hashLen, 0)) {
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return L"";
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	return BytesToHexString(hashBytes.data(), hashLen);
}

std::wstring CryptoHelper::BytesToHexString(const BYTE* bytes, DWORD length) {
	std::wostringstream oss;
	oss << std::hex << std::setfill(L'0');
	
	for (DWORD i = 0; i < length; i++) {
		oss << std::setw(2) << static_cast<int>(bytes[i]);
	}
	
	return oss.str();
}

} // namespace Utils
} // namespace WinProcessInspector
