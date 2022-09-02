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
		, mp_file (NULL)
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

		std::function<int(bool, uint8_t*)> _process = [this] (bool is_proc_inner_buff, uint8_t* p_buffer) {
			if (mp_file == NULL) {
				return -1;
			}

			uint8_t* p = is_proc_inner_buff ? getBuffer() : p_buffer;
			size_t len = is_proc_inner_buff ? getWritedPosition() : getBufferSize();

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
			if (mp_file == NULL) {
				return ;
			}
			fflush (mp_file);
			fclose (mp_file);
		};

		set_process_handler (_process);
		set_release_handler (_release);
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
