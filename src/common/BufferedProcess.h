#ifndef _BUFFERED_PROCESS_H_
#define _BUFFERED_PROCESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memory>
#include <functional>

class CBufferedProcess
{
protected:
	CBufferedProcess (size_t size) 
		: m_process (nullptr)
		, m_buffer_size (0)
		, m_buffered_pos (0)
	{
		m_buffer_size = size;
		std::unique_ptr<uint8_t[]> ubf (new uint8_t[size]);
		m_buffer.swap(ubf);
	}
	
	virtual ~CBufferedProcess (void) {
	}

	void set_process_handler (std::function<int(const uint8_t* p_buffer, size_t length)> _handler) {
		m_process = _handler;
	}

public:
	int put (uint8_t *p_buffer, size_t length) {
		if (m_process == nullptr) {
			//TODO exception
			return -1;
		}

		int r = 0;
		if (m_buffered_pos + length > m_buffer_size) {
			// proccess first inner buffer data
			r = m_process (m_buffer.get(), m_buffered_pos);
			//TODO exception
			if (r < 0) {
				return r;
			}
			m_buffered_pos = 0;
			
			while (length >= m_buffer_size) {
				// directly process the data passed as an argument
				r = m_process (p_buffer, m_buffer_size);
				if (r < 0) {
					//TODO exception
					return r;
				}
				p_buffer += m_buffer_size;
				length -= m_buffer_size;
			}

			// buffering remain data
			buffering (p_buffer, length);
			return 0;

		} else {
			buffering (p_buffer, length);
			return 0;
		}
	}

	void finalize (void) {
		process_remaining();
	}

protected:
	uint8_t* get_buffer() {
		return m_buffer.get();
	}

	size_t get_buffer_size() const {
		return m_buffer_size;
	}

	size_t get_buffered_position() const {
		return m_buffered_pos;
	}

private:
	int process_remaining (void) {
		if (m_process == nullptr) {
			//TODO exception
			return -1;
		}

		int r;
		if (m_buffered_pos > 0) {
			// process remain inner buffer data
			r = m_process(m_buffer.get(), m_buffered_pos);
			if(r < 0)  {
				//TODO exception
				return r;
			}
			m_buffered_pos = 0;
		}

		return 0;
	}

	void buffering (uint8_t *p_buffer, size_t length) {
		if (p_buffer && length > 0) {
			memcpy(m_buffer.get() + m_buffered_pos, p_buffer, length);
			m_buffered_pos += length;
		}
	}

	std::function<int(const uint8_t* p_buffer, size_t length)> m_process;

	std::unique_ptr<uint8_t[]> m_buffer;
	size_t m_buffer_size;
	size_t m_buffered_pos;
};

#endif
