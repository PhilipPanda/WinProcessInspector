#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "NetworkManager.h"
#include <sstream>
#include <iomanip>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace WinProcessInspector {
namespace Core {

std::vector<NetworkConnection> NetworkManager::EnumerateConnections() const {
	std::vector<NetworkConnection> connections;
	
	auto tcpConns = GetTcpConnections();
	connections.insert(connections.end(), tcpConns.begin(), tcpConns.end());
	
	auto udpConns = GetUdpConnections();
	connections.insert(connections.end(), udpConns.begin(), udpConns.end());
	
	auto tcp6Conns = GetTcp6Connections();
	connections.insert(connections.end(), tcp6Conns.begin(), tcp6Conns.end());
	
	auto udp6Conns = GetUdp6Connections();
	connections.insert(connections.end(), udp6Conns.begin(), udp6Conns.end());
	
	return connections;
}

std::vector<NetworkConnection> NetworkManager::GetConnectionsForProcess(DWORD processId) const {
	std::vector<NetworkConnection> allConns = EnumerateConnections();
	std::vector<NetworkConnection> result;
	
	for (const auto& conn : allConns) {
		if (conn.ProcessId == processId) {
			result.push_back(conn);
		}
	}
	
	return result;
}

std::vector<NetworkConnection> NetworkManager::GetTcpConnections() const {
	std::vector<NetworkConnection> connections;
	
	DWORD size = 0;
	if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != ERROR_INSUFFICIENT_BUFFER) {
		return connections;
	}
	
	std::vector<BYTE> buffer(size);
	PMIB_TCPTABLE_OWNER_PID pTcpTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
	
	if (GetExtendedTcpTable(pTcpTable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != NO_ERROR) {
		return connections;
	}
	
	for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
		MIB_TCPROW_OWNER_PID& row = pTcpTable->table[i];
		
		NetworkConnection conn;
		conn.Protocol = NetworkProtocol::TCP;
		conn.ProcessId = row.dwOwningPid;
		conn.LocalAddress = FormatAddress(reinterpret_cast<BYTE*>(&row.dwLocalAddr), false);
		conn.LocalPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
		conn.RemoteAddress = FormatAddress(reinterpret_cast<BYTE*>(&row.dwRemoteAddr), false);
		conn.RemotePort = ntohs(static_cast<USHORT>(row.dwRemotePort));
		conn.State = static_cast<ConnectionState>(row.dwState);
		conn.CreationTime = 0;
		
		connections.push_back(conn);
	}
	
	return connections;
}

std::vector<NetworkConnection> NetworkManager::GetUdpConnections() const {
	std::vector<NetworkConnection> connections;
	
	DWORD size = 0;
	if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) != ERROR_INSUFFICIENT_BUFFER) {
		return connections;
	}
	
	std::vector<BYTE> buffer(size);
	PMIB_UDPTABLE_OWNER_PID pUdpTable = reinterpret_cast<PMIB_UDPTABLE_OWNER_PID>(buffer.data());
	
	if (GetExtendedUdpTable(pUdpTable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) != NO_ERROR) {
		return connections;
	}
	
	for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++) {
		MIB_UDPROW_OWNER_PID& row = pUdpTable->table[i];
		
		NetworkConnection conn;
		conn.Protocol = NetworkProtocol::UDP;
		conn.ProcessId = row.dwOwningPid;
		conn.LocalAddress = FormatAddress(reinterpret_cast<BYTE*>(&row.dwLocalAddr), false);
		conn.LocalPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
		conn.RemoteAddress = L"*";
		conn.RemotePort = 0;
		conn.State = ConnectionState::Closed;
		conn.CreationTime = 0;
		
		connections.push_back(conn);
	}
	
	return connections;
}

std::vector<NetworkConnection> NetworkManager::GetTcp6Connections() const {
	std::vector<NetworkConnection> connections;
	
	DWORD size = 0;
	if (GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) != ERROR_INSUFFICIENT_BUFFER) {
		return connections;
	}
	
	std::vector<BYTE> buffer(size);
	PMIB_TCP6TABLE_OWNER_PID pTcp6Table = reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(buffer.data());
	
	if (GetExtendedTcpTable(pTcp6Table, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) != NO_ERROR) {
		return connections;
	}
	
	for (DWORD i = 0; i < pTcp6Table->dwNumEntries; i++) {
		MIB_TCP6ROW_OWNER_PID& row = pTcp6Table->table[i];
		
		NetworkConnection conn;
		conn.Protocol = NetworkProtocol::TCPv6;
		conn.ProcessId = row.dwOwningPid;
		conn.LocalAddress = FormatAddress(row.ucLocalAddr, true);
		conn.LocalPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
		conn.RemoteAddress = FormatAddress(row.ucRemoteAddr, true);
		conn.RemotePort = ntohs(static_cast<USHORT>(row.dwRemotePort));
		conn.State = static_cast<ConnectionState>(row.dwState);
		conn.CreationTime = 0;
		
		connections.push_back(conn);
	}
	
	return connections;
}

std::vector<NetworkConnection> NetworkManager::GetUdp6Connections() const {
	std::vector<NetworkConnection> connections;
	
	DWORD size = 0;
	if (GetExtendedUdpTable(nullptr, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) != ERROR_INSUFFICIENT_BUFFER) {
		return connections;
	}
	
	std::vector<BYTE> buffer(size);
	PMIB_UDP6TABLE_OWNER_PID pUdp6Table = reinterpret_cast<PMIB_UDP6TABLE_OWNER_PID>(buffer.data());
	
	if (GetExtendedUdpTable(pUdp6Table, &size, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) != NO_ERROR) {
		return connections;
	}
	
	for (DWORD i = 0; i < pUdp6Table->dwNumEntries; i++) {
		MIB_UDP6ROW_OWNER_PID& row = pUdp6Table->table[i];
		
		NetworkConnection conn;
		conn.Protocol = NetworkProtocol::UDPv6;
		conn.ProcessId = row.dwOwningPid;
		conn.LocalAddress = FormatAddress(row.ucLocalAddr, true);
		conn.LocalPort = ntohs(static_cast<USHORT>(row.dwLocalPort));
		conn.RemoteAddress = L"*";
		conn.RemotePort = 0;
		conn.State = ConnectionState::Closed;
		conn.CreationTime = 0;
		
		connections.push_back(conn);
	}
	
	return connections;
}

std::wstring NetworkManager::FormatAddress(const BYTE* addr, bool isIPv6) const {
	wchar_t buffer[INET6_ADDRSTRLEN] = {};
	
	if (isIPv6) {
		IN6_ADDR addr6;
		memcpy(&addr6, addr, sizeof(IN6_ADDR));
		InetNtopW(AF_INET6, &addr6, buffer, INET6_ADDRSTRLEN);
	} else {
		IN_ADDR addr4;
		memcpy(&addr4, addr, sizeof(IN_ADDR));
		InetNtopW(AF_INET, &addr4, buffer, INET6_ADDRSTRLEN);
	}
	
	return std::wstring(buffer);
}

std::wstring NetworkManager::GetStateString(ConnectionState state) {
	switch (state) {
		case ConnectionState::Closed: return L"Closed";
		case ConnectionState::Listen: return L"Listening";
		case ConnectionState::SynSent: return L"SYN Sent";
		case ConnectionState::SynReceived: return L"SYN Received";
		case ConnectionState::Established: return L"Established";
		case ConnectionState::FinWait1: return L"FIN Wait 1";
		case ConnectionState::FinWait2: return L"FIN Wait 2";
		case ConnectionState::CloseWait: return L"Close Wait";
		case ConnectionState::Closing: return L"Closing";
		case ConnectionState::LastAck: return L"Last ACK";
		case ConnectionState::TimeWait: return L"Time Wait";
		case ConnectionState::DeleteTcb: return L"Delete TCB";
		default: return L"Unknown";
	}
}

std::wstring NetworkManager::GetProtocolString(NetworkProtocol protocol) {
	switch (protocol) {
		case NetworkProtocol::TCP: return L"TCP";
		case NetworkProtocol::UDP: return L"UDP";
		case NetworkProtocol::TCPv6: return L"TCPv6";
		case NetworkProtocol::UDPv6: return L"UDPv6";
		default: return L"Unknown";
	}
}

} // namespace Core
} // namespace WinProcessInspector
