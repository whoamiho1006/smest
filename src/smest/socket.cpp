
#include "socket.hpp"

#if PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 0

#include <WinSock2.h>
#include <Windows.h>

typedef smest::int32 socklen_t;
inline smest::int32 poll(pollfd* fds, smest::int32 len, smest::int32 timeout) {
	return WSAPoll(fds, len, timeout);
}
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace smest {

#if PLATFORM_WINDOWS
	class wsa_init_t {
	private:
		WSADATA m_Data;

	private:
		wsa_init_t() {
			memset(&m_Data, 0, sizeof(m_Data));
			WSAStartup(MAKEWORD(2, 2), &m_Data);
		}

		~wsa_init_t() { WSACleanup(); }

	public:
		inline static void init() {
			static wsa_init_t init;
		}
	};

	inline static void socket_init() {
		wsa_init_t::init();
	}
#else
	inline static void socket_init() { }
#endif

	int32 socket_base_t::poll_group(const poll_group_t& sockets, poll_group_t& reads, 
		poll_group_t& writes, poll_group_t& errors, int32 timeout)
	{
		pollfd* fds = new pollfd[sockets.size()];
		memset(fds, 0, sizeof(pollfd) * sockets.size());

		for (uint32 i = 0; i < sockets.size(); i++) {
			fds[i].fd = sockets[i]->fd();
			fds[i].events = POLLIN | POLLOUT;
		}

		int32 retVal = ::poll(fds, sockets.size(), timeout);
		
		if (retVal > 0) {
			for (uint32 i = 0; i < sockets.size(); i++) {
				if ((fds[i].revents & POLLERR) == POLLERR ||
					(fds[i].revents & POLLHUP) == POLLHUP)
					errors.push_back(sockets[i]);

				else {
					if ((fds[i].revents & POLLIN) == POLLIN)
						reads.push_back(sockets[i]);

					if ((fds[i].revents & POLLOUT) == POLLOUT)
						writes.push_back(sockets[i]);
				}
			}
		}

		delete[] fds;
		return retVal;
	}

	bool socket_tcp_t::no_delay(bool val) {
		int32 value = val ? 1 : 0;
		int32 retVal = setsockopt(m_fd, IPPROTO_TCP, 
			TCP_NODELAY, (const char*)&value, sizeof(value));

		return !retVal;
	}

	bool socket_tcp_t::keep_alive(bool val) {
		int32 value = val ? 1 : 0;
		int32 retVal = setsockopt(m_fd, SOL_SOCKET,
			SO_KEEPALIVE, (const char*)&value, sizeof(value));

		return !retVal;
	}

	bool socket_tcp_t::init() {
		socket_init();

		if (m_fd >= 0) {
			return false;
		}

		return (m_fd = socket(AF_INET, SOCK_STREAM, 0)) >= 0;
	}

	bool socket_udp_t::init() {
		socket_init();

		if (m_fd >= 0) {
			return false;
		}

		return (m_fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0;
	}

	bool socket_tcp_t::connect(const char* host, short port, int32 timeout)
	{
		if (m_fd < 0 && !init()) {
			return false;
		}

		hostent* servhost = gethostbyname(host);
		sockaddr_in addr;

		m_error.clear();
		if (!servhost) {
			m_error = std::string("error in gethostbyname: ") + host;
			return false;
		}

		addr.sin_family = AF_INET; addr.sin_port = htons(port);
		memcpy(&addr.sin_addr, servhost->h_addr, servhost->h_length);

		if (timeout < 0) {
			if (::connect(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
				m_error = strerror(errno);
				return false;
			}
		}

		else {
			bool succeed = true;

#if PLATFORM_WINDOWS
			unsigned long ul = 1;
			ioctlsocket(m_fd, FIONBIO, &ul);
#else
			int32 arg;

			if ((arg = ::fcntl(m_fd, F_GETFL, 0)) < 0) {
				m_error = strerror(errno);
				return false;
			}

			arg |= O_NONBLOCK;

			if (::fcntl(m_fd, F_SETFL, arg) < 0) {
				m_error = strerror(errno);
				return false;
			}
#endif

			while (true) {
				if (::connect(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
					if (!errno || errno == EINPROGRESS || errno == EAGAIN) {
						pollfd pfd;

						pfd.fd = m_fd;
						pfd.events = POLLIN | POLLOUT;
						pfd.revents = 0;

						int32 retVal = ::poll(&pfd, 1, timeout);

						if (retVal <= 0) {
							m_error = strerror(errno);
							succeed = false;
							break;
						}

						socklen_t lon = sizeof(int32);

						if (::getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char*)&retVal, &lon)) {
							m_error = strerror(errno);
							succeed = false;
							break;
						}

						if (retVal) {
							m_error = strerror(retVal);
							succeed = false;
							break;
						}
					}
					else {
						m_error = strerror(errno);
						succeed = false;
						break;
					}
				}

				break;
			}

#if PLATFORM_WINDOWS
			ul = 0;
			ioctlsocket(m_fd, FIONBIO, &ul);
#else
			if ((arg = ::fcntl(m_fd, F_GETFL, 0)) < 0) {
				m_error = strerror(errno);
				return false;
			}

			arg &= ~O_NONBLOCK;

			if (::fcntl(m_fd, F_SETFL, arg) < 0) {
				m_error = strerror(errno);
				return false;
			}
#endif

			return succeed;
		}

		return true;
	}

	bool socket_tcp_t::bind(const uint8* address, short port) {
		sockaddr_in addr;
		
		addr.sin_family = AF_INET; addr.sin_port = htons(port);
		memcpy(&addr.sin_addr, address, sizeof(uint8) * 4);

		int32 retVal = ::bind(m_fd, (const sockaddr*)&addr, sizeof(addr));

		m_error.clear();
		if (retVal) {
			m_error = strerror(errno);
		}

		return !retVal;
	}

	bool socket_udp_t::bind(const uint8* address, short port) {
		sockaddr_in addr;

		addr.sin_family = AF_INET; addr.sin_port = htons(port);
		memcpy(&addr.sin_addr, address, sizeof(uint8) * 4);

		int32 retVal = ::bind(m_fd, (const sockaddr*)&addr, sizeof(addr));

		m_error.clear();
		if (retVal) {
			m_error = strerror(errno);
		}

		return !retVal;
	}

	bool socket_tcp_t::listen(uint32 backlog) {
		int32 retVal = ::listen(m_fd, backlog);

		m_error.clear();
		if (retVal) {
			m_error = strerror(errno);
		}

		return !retVal;
	}

	socket_tcp_t socket_tcp_t::accept() {
		sockaddr_in addr;
		socklen_t len;
		
		int32 fd = ::accept(m_fd, (sockaddr*)&addr, &len);

		m_error.clear();
		if (fd < 0) {
			m_error = strerror(errno);
		}

		return fd;
	}

	int32 socket_tcp_t::send(const void* buffer, int32 size) {
		int32 retVal = ::send(m_fd, (const char*)buffer, size, 0);

		m_error.clear();
		if (retVal < 0) {
			m_error = strerror(errno);
		}

		return retVal;
	}

	int32 socket_udp_t::send_to(const end_point_t& addr, const void* buffer, int32 size) {
		sockaddr_in sadr;

		sadr.sin_family = AF_INET; sadr.sin_port = htons(addr.port);
		memcpy(&sadr.sin_addr, addr.address, sizeof(uint8) * 4);

		int32 retVal = ::sendto(m_fd, (const char*)buffer, 
			size, 0, (const sockaddr*)&sadr, sizeof(sadr));

		m_error.clear();
		if (retVal < 0) {
			m_error = strerror(errno);
		}

		return retVal;
	}

	int32 socket_tcp_t::recv(void* buffer, int32 size) {
		int32 retVal = ::recv(m_fd, (char*)buffer, size, 0);

		m_error.clear();
		if (retVal < 0) {
			m_error = strerror(errno);
		}

		return retVal;
	}

	int32 socket_udp_t::recv_from(end_point_t& addr, void* buffer, int32 size) {
		sockaddr_in sadr = { 0, };
		socklen_t slen = sizeof(sadr);

		int32 retVal = ::recvfrom(m_fd, (char*)buffer,
			size, 0, (sockaddr*)&sadr, &slen);

		m_error.clear();
		if (retVal < 0) {
			m_error = strerror(errno);
		}
		else {
			memcpy(addr.address, &sadr.sin_addr, sizeof(uint8) * 4);
			addr.port = ntohs(sadr.sin_port);
		}

		return retVal;
	}

	int32 socket_base_t::poll(int32 type) {
		pollfd pfd;

		if (m_fd < 0) {
			return -1;
		}

		pfd.fd = m_fd;
		pfd.revents = 0;
		pfd.events = 0;

		if ((type & ESPOLL_READ) != 0)
			pfd.events |= POLLIN;

		if ((type & ESPOLL_WRITE) != 0)
			pfd.events |= POLLOUT;

		if (!pfd.events) {
			return -1;
		}

		int32 retVal = ::poll(&pfd, 1, 0);

		if (!retVal) {
			m_error = strerror(errno);
			return -1;
		}

		type = 0;
		if ((pfd.revents & POLLIN) == POLLIN)
			type |= ESPOLL_READ;

		if ((pfd.revents & POLLOUT) == POLLOUT)
			type |= ESPOLL_WRITE;

		if ((pfd.revents & POLLERR) == POLLERR ||
			(pfd.revents & POLLHUP) == POLLHUP) 
		{
			close();
			return -1;
		}

		return type;
	}

	bool socket_base_t::close() {
		if (m_fd >= 0) {
#if PLATFORM_WINDOWS
			::closesocket(m_fd);
#else
			::close(m_fd);
#endif

			m_fd = -1;
			m_error.clear();
			return true;
		}

		return false;
	}
}