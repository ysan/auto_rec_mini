#ifndef _STACK_H_
#define _STACK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define STACK_DEPTH		(16)

template <typename T>
class CStack
{
public:
	CStack (void) {
		m_sp = 0;
		clear ();
	}
	virtual ~CStack (void) {
		m_sp = 0;
		clear ();
	}


	void push (T *p) {
		if (!p) {
			return;
		}
		if (m_sp == STACK_DEPTH) {
			return;
		}

		mp_stack [m_sp] = p;
		//printf ("push %d %p\n", m_sp, mp_stack [m_sp]);
		++ m_sp;
	}

	T* pop (void) {
		if (m_sp == 0) {
			return NULL;
		}

		T *r = mp_stack [m_sp -1];
		//printf ("pop %d %p\n", m_sp -1, mp_stack [m_sp -1]);
		mp_stack [m_sp -1] = NULL;
		-- m_sp;

		return r;
	}

	T* peep (void) {
		if (m_sp == 0) {
			return NULL;
		}

		return mp_stack [m_sp -1];
	}

	T* ref (int idx) {
		if ((idx < 0) || (idx >= STACK_DEPTH)) {
			return NULL;
		}

		return mp_stack [idx];
	}

	int get_sp (void) {
		return m_sp ;
	}

	void clear (void) {
		for (int i =0; i < STACK_DEPTH; ++ i) {
			mp_stack [i] = NULL;
		}

		m_sp = 0;
	}

private:

	T *mp_stack [STACK_DEPTH];
	int m_sp;

};

#endif
