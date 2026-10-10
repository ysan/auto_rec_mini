#ifndef _STACK_H_
#define _STACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

namespace arec {
namespace common {



#define STACK_DEPTH		(16)

template <typename T>
class stack
{
public:
	stack (void) {
		sp_ = 0;
		clear ();
	}
	virtual ~stack (void) {
		sp_ = 0;
		clear ();
	}


	void push (T *p) {
		if (!p) {
			return;
		}
		if (sp_ == STACK_DEPTH) {
			return;
		}

		mp_stack [sp_] = p;
		//printf ("push %d %p\n", sp_, mp_stack [sp_]);
		++ sp_;
	}

	T* pop (void) {
		if (sp_ == 0) {
			return NULL;
		}

		T *r = mp_stack [sp_ -1];
		//printf ("pop %d %p\n", sp_ -1, mp_stack [sp_ -1]);
		mp_stack [sp_ -1] = NULL;
		-- sp_;

		return r;
	}

	T* peep (void) {
		if (sp_ == 0) {
			return NULL;
		}

		return mp_stack [sp_ -1];
	}

	T* ref (int idx) {
		if ((idx < 0) || (idx >= STACK_DEPTH)) {
			return NULL;
		}

		return mp_stack [idx];
	}

	int get_sp (void) {
		return sp_ ;
	}

	void clear (void) {
		for (int i =0; i < STACK_DEPTH; ++ i) {
			mp_stack [i] = NULL;
		}

		sp_ = 0;
	}

private:

	T *mp_stack [STACK_DEPTH];
	int sp_;

};


} // namespace common
} // namespace arec

#endif
