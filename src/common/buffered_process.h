#ifndef _BUFFERED_PROCESS_H_
#define _BUFFERED_PROCESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memory>
#include <functional>

namespace arec {
namespace common {


class buffered_process
{
protected:
	buffered_process (size_t size) 
		: process_ (nullptr)
		, buffer_size_ (0)
		, buffered_pos_ (0)
	{
		buffer_size_ = size;
		std::unique_ptr<uint8_t[]> ubf (new uint8_t[size]);
		buffer_.swap(ubf);
	}
	
	virtual ~buffered_process (void) {
	}

	void set_process_handler (std::function<int(const uint8_t* p_buffer, size_t length)> _handler) {
		process_ = _handler;
	}

public:
	int put (uint8_t *p_buffer, size_t length) {
		if (process_ == nullptr) {
			//TODO exception
			return -1;
		}

		int r = 0;
		if (buffered_pos_ + length > buffer_size_) {
			// proccess first inner buffer data
			r = process_ (buffer_.get(), buffered_pos_);
			//TODO exception
			if (r < 0) {
				return r;
			}
			buffered_pos_ = 0;
			
			while (length >= buffer_size_) {
				// directly process the data passed as an argument
				r = process_ (p_buffer, buffer_size_);
				if (r < 0) {
					//TODO exception
					return r;
				}
				p_buffer += buffer_size_;
				length -= buffer_size_;
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
		return buffer_.get();
	}

	size_t get_buffer_size() const {
		return buffer_size_;
	}

	size_t get_buffered_position() const {
		return buffered_pos_;
	}

private:
	int process_remaining (void) {
		if (process_ == nullptr) {
			//TODO exception
			return -1;
		}

		int r;
		if (buffered_pos_ > 0) {
			// process remain inner buffer data
			r = process_(buffer_.get(), buffered_pos_);
			if(r < 0)  {
				//TODO exception
				return r;
			}
			buffered_pos_ = 0;
		}

		return 0;
	}

	void buffering (uint8_t *p_buffer, size_t length) {
		if (p_buffer && length > 0) {
			memcpy(buffer_.get() + buffered_pos_, p_buffer, length);
			buffered_pos_ += length;
		}
	}

	std::function<int(const uint8_t* p_buffer, size_t length)> process_;

	std::unique_ptr<uint8_t[]> buffer_;
	size_t buffer_size_;
	size_t buffered_pos_;
};


} // namespace common
} // namespace arec

#endif
