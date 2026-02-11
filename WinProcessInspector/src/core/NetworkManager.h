#pragma once

#include <vector>
#include <string>
#include <Windows.h>

namespace WinProcessInspector {
namespace Core {

	enum class NetworkProtocol {
		TCP,
		UDP,
		TCPv6,
		UDPv6
	};

	enum class ConnectionState {
		Closed = 1,
		Listen = 2,
		SynSent = 3,
		SynReceived = 4,
		Established = 5,
		FinWait1 = 6,
		FinWait2 = 7,
		CloseWait = 8,
		Closing = 9,
		LastAck = 10,
		TimeWait = 11,
		DeleteTcb = 12
	};

	struct NetworkConnection {
		NetworkProtocol Protocol;
		DWORD ProcessId;
		std::wstring ProcessName;
		std::wstring LocalAddress;
		DWORD LocalPort;
		std::wstring RemoteAddress;
		DWORD RemotePort;
		ConnectionState State;
		ULONGLONG CreationTime;
	};

	class NetworkManager {
	public:
		NetworkManager() = default;
		~NetworkManager() = default;

		NetworkManager(const NetworkManager&) = delete;
		NetworkManager& operator=(const NetworkManager&) = delete;
		NetworkManager(NetworkManager&&) = default;
		NetworkManager& operator=(NetworkManager&&) = default;

		std::vector<NetworkConnection> EnumerateConnections() const;
		std::vector<NetworkConnection> GetConnectionsForProcess(DWORD processId) const;
		
		static std::wstring GetStateString(ConnectionState state);
		static std::wstring GetProtocolString(NetworkProtocol protocol);

	private:
		std::vector<NetworkConnection> GetTcpConnections() const;
		std::vector<NetworkConnection> GetUdpConnections() const;
		std::vector<NetworkConnection> GetTcp6Connections() const;
		std::vector<NetworkConnection> GetUdp6Connections() const;
		
		std::wstring FormatAddress(const BYTE* addr, bool isIPv6) const;
	};

} // namespace Core
} // namespace WinProcessInspector
