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
		, m_finalize (nullptr)
		, m_buffer_size (0)
		, m_processed_pos (0)
	{
		m_buffer_size = size;
		std::unique_ptr<uint8_t[]> ubf (new uint8_t[size]);
		m_buffer.swap(ubf);
	}
	
	virtual ~CBufferedProcess (void) {
	}

	void set_process_handler (std::function<int(bool need_proc_inner_buff, uint8_t* p_buffer)> _handler) {
		m_process = _handler;
	}
	void set_finalize_handler (std::function<void(void)> _handler) {
		m_finalize = _handler;
	}

public:
	virtual int put (uint8_t *p_buffer, size_t length) {
		if (m_process == nullptr) {
			return -1;
		}

		int r = 0;
		if (m_processed_pos + length > m_buffer_size) {
			// proccess first inner buffer data
			r = m_process (true, nullptr);
			if (r < 0) {
				return r;
			}
			m_processed_pos = 0;
			
			while (length >= m_buffer_size) {
				// directly process the data passed as an argument
				r = m_process (false, p_buffer);
				if (r < 0) {
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

	virtual int process_remaining (void) {
		if (m_process == nullptr) {
			return -1;
		}

		int r;
		if (m_processed_pos > 0) {
			// process remain inner buffer data
			r = m_process(true, nullptr);
			if(r < 0)  {
				return r;
			}
			m_processed_pos = 0;
		}

		return 0;
	}

	virtual void finalize (void) {
		if (m_finalize == nullptr) {
			return;
		}

		m_finalize ();
	}

protected:
	uint8_t* get_buffer() {
		return m_buffer.get();
	}

	size_t get_buffer_size() const {
		return m_buffer_size;
	}

	size_t get_processed_position() const {
		return m_processed_pos;
	}

private:
	void buffering (uint8_t *p_buffer, size_t length) {
		if (p_buffer && length > 0) {
			memcpy(m_buffer.get() + m_processed_pos, p_buffer, length);
			m_processed_pos += length;
		}
	}

	std::function<int(bool need_proc_inner_buff, uint8_t* p_buffer)> m_process;
	std::function<void()> m_finalize;

	std::unique_ptr<uint8_t[]> m_buffer;
	size_t m_buffer_size;
	size_t m_processed_pos;
};

#endif
