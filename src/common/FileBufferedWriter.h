#ifndef _FILE_BUFFERED_WRITER_H_
#define _FILE_BUFFERED_WRITER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "BufferedProcess.h"

class CFileBufferedWriter : public CBufferedProcess
{
public:
	CFileBufferedWriter (size_t size, std::string output_name) 
		: CBufferedProcess (size)
		, mp_file (nullptr)
	{
		init(output_name);
	}
	
	virtual ~CFileBufferedWriter (void) {
	}

	void init (const std::string &output_name) {

		if (!output_name.empty()) {
			mp_file = fopen(output_name.c_str(), "w");
			if (!mp_file) {
				perror ("fopen failure");
				return;
			}
		}

		std::function<int(bool, uint8_t*)> _process = [this] (bool need_proc_inner_buff, uint8_t* p_buffer) {
			if (mp_file == nullptr) {
				return -1;
			}

			uint8_t* p = need_proc_inner_buff ? get_buffer() : p_buffer;
			size_t len = need_proc_inner_buff ? get_buffered_position() : get_buffer_size();

			int r = 0;
			while (len > 0) {
				r = fwrite (p, 1, len, mp_file);
				if (r < 0) {
					return r;
				}
				len -= r;
			}

			return 0;
		};

		std::function<void(void)> _release = [this] (void) {
			if (mp_file == nullptr) {
				return ;
			}
			fflush (mp_file);
			fclose (mp_file);
		};

		set_process_handler (_process);
		set_finalize_handler (_release);
	}

	int flush (void) {
		return process_remaining();
	}

	void release (void) {
		finalize();
	}

private:

	FILE *mp_file;
};

#endif

#if 0
// test code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "FileBufferedWriter.h"

int main (int argc, char *argv[])
{
	std::cout << argv[1] << std::endl;

	std::ifstream ifs (argv[1], std::ios::in);
	std::stringstream ss;
	ss << ifs.rdbuf();
//	std::cout << ss.str() << std::endl;
	

	CFileBufferedWriter writer (128, std::string("out"));
	writer.put((uint8_t*)ss.str().c_str(), ss.str().length());
	writer.flush();
	writer.release();

	ifs.close();
	ss.clear();
	return 0;
}
#endif
