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
		release();
	}

	void init (const std::string &output_name) {

		if (!output_name.empty()) {
			mp_file = fopen(output_name.c_str(), "w");
			if (!mp_file) {
				perror ("fopen failure");
				return;
			}
		}

		std::function<int(const uint8_t*, size_t)> _process = [this] (const uint8_t* p_buffer, size_t  length) {
			if (mp_file == nullptr) {
				return -1;
			}

			uint8_t* p = const_cast<uint8_t*>(p_buffer);
			size_t len = length;

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

		set_process_handler (_process);
	}

	void flush (void) {
		CBufferedProcess::finalize();
		fflush (mp_file);
	}

private:
	void release (void) {
		if (mp_file == nullptr) {
			return ;
		}
		fclose (mp_file);
		mp_file = nullptr;
	}


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

	ifs.close();
	ss.clear();
	return 0;
}
#endif
