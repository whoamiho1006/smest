#pragma once
#include "types.hpp"

namespace smest {

	enum ESocketPoll {
		ESPOLL_READ = 1,
		ESPOLL_WRITE = 2,
		ESPOLL_BOTH = 3
	};

	class socket_base_t;
	typedef std::vector<socket_base_t*> poll_group_t;

	/*
		Socket class library.
	*/
	class socket_base_t {
	protected:
		string m_error;
		int32 m_fd;

	public:
		socket_base_t() : m_fd(-1) { }
		socket_base_t(int32 fd) : m_fd(fd) { }

		virtual ~socket_base_t() {
			if (m_fd >= 0) {
				close();
			}
		}

	public:
		inline bool is_valid() const { return m_fd >= 0; }

		inline int32 fd() const { return m_fd; }
		inline const string& error() const { return m_error; }

	public:
		static int32 poll_group(const poll_group_t& sockets, poll_group_t& reads, 
			poll_group_t& writes, poll_group_t& errors, int32 timeout);

		/*
			Connect/Bind/Listen must be in implementator.
		*/
	public:
		virtual bool init() = 0;

		virtual int32 send(const void* buffer, int32 size) = 0;
		virtual int32 recv(void* buffer, int32 size) = 0;

		virtual int32 poll(int32 type = ESPOLL_BOTH);

		/* close socket. */
		virtual bool close();
	};

	/* Internet Protocol Socket class. -> TCP only supported. */
	class socket_tcp_t : public socket_base_t {
	public:
		socket_tcp_t() { }
		socket_tcp_t(int32 fd) : socket_base_t(fd) { }

		socket_tcp_t(const socket_tcp_t& other) : socket_tcp_t(other.m_fd) { }
		socket_tcp_t(socket_tcp_t&& other) : socket_tcp_t(other.m_fd) { other.m_fd = -1; }

	public:
		inline socket_tcp_t& operator =(const socket_tcp_t& other) {
			if (&other != this) {
				if (m_fd >= 0) {
					close();
				}

				m_fd = other.m_fd;
			}

			return *this;
		}

		inline socket_tcp_t& operator =(socket_tcp_t&& other) {
			int32 fd = m_fd;

			m_fd = other.m_fd;
			other.m_fd = fd;

			return *this;
		}

	public:
		bool no_delay(bool val);
		bool keep_alive(bool val);

	public:
		virtual bool init() final;

		/* connect to host. */
		bool connect(const char* host, short port, int32 timeout);

		/* bind socket to address.*/
		bool bind(const uint8* address, short port);

		/* listen socket. */
		bool listen(uint32 backlog = 10);

		/* accept socket. */
		socket_tcp_t accept();

		/* send bytes to host. */
		virtual int32 send(const void* buffer, int32 size) final;

		/* receive bytes from host. */
		virtual int32 recv(void* buffer, int32 size) final;

	};

	class socket_udp_t : public socket_base_t {
	public:
		socket_udp_t() { }
		socket_udp_t(int32 fd) : socket_base_t(fd) { }

		socket_udp_t(const socket_udp_t& other) : socket_udp_t(other.m_fd) { }
		socket_udp_t(socket_udp_t&& other) : socket_udp_t(other.m_fd) { other.m_fd = -1; }

	public:
		inline socket_udp_t& operator =(const socket_udp_t& other) {
			if (&other != this) {
				if (m_fd >= 0) {
					close();
				}

				m_fd = other.m_fd;
			}

			return *this;
		}

		inline socket_udp_t& operator =(socket_udp_t&& other) {
			int32 fd = m_fd;

			m_fd = other.m_fd;
			other.m_fd = fd;

			return *this;
		}

	public:
		struct end_point_t {
			uint8 address[4];
			short port;
		};

	public:
		virtual bool init() final;

		/* bind socket to address.*/
		bool bind(const uint8* address, short port);

		/* send bytes to host. */
		virtual int32 send_to(const end_point_t& addr, const void* buffer, int32 size) final;

		/* receive bytes from host. */
		virtual int32 recv_from(end_point_t& addr, void* buffer, int32 size) final;
	};
}