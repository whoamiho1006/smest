#pragma once
#include "types.hpp"

namespace smest {

	class packet_t {
	protected:
		uint8* m_buffer;
		size_t m_length;

	public:
		packet_t() : m_buffer(0), m_length(0) { }
		packet_t(const packet_t& other)
			: m_buffer(0), m_length(0)
		{
			if (other.m_buffer) {
				m_buffer = new uint8[other.m_length];
				std::memcpy(m_buffer, other.m_buffer, other.m_length);
			}
		}

		packet_t(packet_t&& other)
			: m_buffer(other.m_buffer), m_length(other.m_length)
		{
			other.m_buffer = 0;
			other.m_length = 0;
		}

		~packet_t() { length(0); }

	public:
		inline packet_t& operator =(const packet_t& other) {
			if (&other != this) {
				length(0);
				
				if (other.length() && (m_buffer = new uint8[other.length()]) != nullptr) 
					memcpy(m_buffer, other.m_buffer, m_length = other.length());
			}

			return *this;
		}

		inline packet_t& operator =(packet_t&& other) {
			auto* temp1 = m_buffer;
			auto temp2 = m_length;

			m_buffer = other.m_buffer;
			m_length = other.m_length;

			other.m_buffer = temp1;
			other.m_length = temp2;
			return *this;
		}

	public:
		inline uint8* buffer() const { return m_buffer; }
		inline size_t length() const { return m_length; }

	public:
		inline bool length(size_t length) {
			if (m_length != length) {
				if (length) {
					/* test null against mem-lack. */
					if (uint8* newBuffer = new uint8[length]) {
						if (m_buffer) {
							size_t offset = length > m_length ? m_length : length;

							memcpy(newBuffer, m_buffer, offset);
							memset(newBuffer + offset, 0, length - offset);

							delete[] m_buffer;
						}

						m_buffer = newBuffer;
						m_length = length;
						return true;
					}

					return false;
				}

				if (m_buffer) {
					delete[] m_buffer;
				}

				m_buffer = 0;
				m_length = 0;
				return true;
			}

			return false;
		}
	};

	enum class packet_endian {
		as_is, little, big
	};

	class packet_writer_t {
	protected:
		bool m_reverseBytes;
		packet_endian m_endian;

		packet_t* m_packet;
		int32_t m_offset;

	public:
		packet_writer_t(packet_t& packet, packet_endian endian = packet_endian::as_is) 
			: m_packet(&packet), m_offset(0), m_endian(endian)
		{ 
			m_reverseBytes = (endian != packet_endian::as_is);
			if (m_reverseBytes) {
				uint8 tester[] = { 0, 123 };

				switch (endian) {
				case packet_endian::as_is:
					m_reverseBytes = false;
					break;

				case packet_endian::little:
					m_reverseBytes = *((uint16*)tester) == 123;
					break;

				case packet_endian::big:
					m_reverseBytes = *((uint16*)tester) != 123;
					break;
				}
				
			}
		}

		virtual ~packet_writer_t() { }

	public:
		inline packet_endian endian() const { return m_endian; }
		inline int32 offset() const { return m_offset; }

		inline packet_t& packet() { return *m_packet; }
		inline const packet_t& packet() const { return *m_packet; }

		inline void offset(int32 val) {
			m_offset = m_packet->length() > val ? 
				val : m_packet->length();
		}

		inline void endian(packet_endian val) {
			if (val != m_endian) {
				if (m_endian != packet_endian::as_is) {
					m_reverseBytes = !m_reverseBytes;
					m_endian = val;
				}

				else {
					uint8 tester[] = { 0, 123 };

					switch (val) {
					case packet_endian::as_is:
						m_reverseBytes = false;
						break;

					case packet_endian::little:
						m_reverseBytes = *((uint16*)tester) == 123;
						break;

					case packet_endian::big:
						m_reverseBytes = *((uint16*)tester) != 123;
						break;
					}
				}
			}
		}

	private:
		inline void reverse_bytes(uint8* bytes, size_t length) {
			for (size_t i = 0; i < length / 2; i++) {
				uint8 temp = bytes[i];
				bytes[i] = bytes[length - (i + 1)];
				bytes[length - (i + 1)] = temp;
			}
		}

		inline bool write_endian_bytes(uint8* bytes, size_t length) {
			if (m_reverseBytes) {
				reverse_bytes(bytes, length);
			}

			return write_bytes(bytes, length);
		}

	public:
		inline bool write_bytes(uint8* bytes, size_t length) {
			if (m_packet->length() < m_offset + length) {
				if (m_packet->length(m_offset + length))
					return false;
			}

			memcpy(m_packet->buffer() + m_offset, bytes, length);
			m_offset += length;

			return true;
		}

#define DECLARE_PACKET_WRITE_METHOD(type, stub) \
		inline bool write_##type(type val) { return stub((uint8*)&val, sizeof(val)); }

	public:
		DECLARE_PACKET_WRITE_METHOD(uint8, write_bytes);
		DECLARE_PACKET_WRITE_METHOD(uint16, write_endian_bytes);
		DECLARE_PACKET_WRITE_METHOD(uint32, write_endian_bytes);
		DECLARE_PACKET_WRITE_METHOD(uint64, write_endian_bytes);

		DECLARE_PACKET_WRITE_METHOD(int8, write_bytes);
		DECLARE_PACKET_WRITE_METHOD(int16, write_endian_bytes);
		DECLARE_PACKET_WRITE_METHOD(int32, write_endian_bytes);
		DECLARE_PACKET_WRITE_METHOD(int64, write_endian_bytes);
	};

	class packet_reader_t {
	protected:
		bool m_reverseBytes;
		packet_endian m_endian;

		const packet_t* m_packet;
		int32_t m_offset;

	public:
		packet_reader_t(const packet_t& packet, packet_endian endian = packet_endian::as_is)
			: m_packet(&packet), m_offset(0), m_endian(endian)
		{
			m_reverseBytes = (endian != packet_endian::as_is);
			if (m_reverseBytes) {
				uint8 tester[] = { 0, 123 };

				switch (endian) {
				case packet_endian::as_is:
					m_reverseBytes = false;
					break;

				case packet_endian::little:
					m_reverseBytes = *((uint16*)tester) == 123;
					break;

				case packet_endian::big:
					m_reverseBytes = *((uint16*)tester) != 123;
					break;
				}

			}
		}

		virtual ~packet_reader_t() { }


	private:
		inline void reverse_bytes(uint8* bytes, size_t length) {
			for (size_t i = 0; i < length / 2; i++) {
				uint8 temp = bytes[i];
				bytes[i] = bytes[length - (i + 1)];
				bytes[length - (i + 1)] = temp;
			}
		}

		inline bool read_endian_bytes(uint8* bytes, size_t length) {
			if (read_bytes(bytes, length)) {
				if (m_reverseBytes) {
					reverse_bytes(bytes, length);
				}

				return true;
			}

			return false;
		}

	public:
		inline bool read_bytes(uint8* bytes, size_t length) {
			if (m_packet->length() < m_offset + length) {
				return false;
			}

			memcpy(bytes, m_packet->buffer() + m_offset, length);
			m_offset += length;

			return true;
		}

#define DECLARE_PACKET_READ_METHOD(type, stub) \
		inline bool try_read_##type(type& val) { return stub((uint8*)&val, sizeof(val)); } \
		inline type read_##type(type def = 0) { \
			try_read_##type(def); return def; \
		}

	public:
		DECLARE_PACKET_READ_METHOD(uint8, read_bytes);
		DECLARE_PACKET_READ_METHOD(uint16, read_endian_bytes);
		DECLARE_PACKET_READ_METHOD(uint32, read_endian_bytes);
		DECLARE_PACKET_READ_METHOD(uint64, read_endian_bytes);

		DECLARE_PACKET_READ_METHOD(int8, read_bytes);
		DECLARE_PACKET_READ_METHOD(int16, read_endian_bytes);
		DECLARE_PACKET_READ_METHOD(int32, read_endian_bytes);
		DECLARE_PACKET_READ_METHOD(int64, read_endian_bytes);
	};
}