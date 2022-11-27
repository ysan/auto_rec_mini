#ifndef _SHARED_H_
#define _SHARED_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <exception>

template <typename T = void>
class CShared {
public:
	CShared (void) = default;
	virtual ~CShared (void) = default;

	bool push (std::string key, std::shared_ptr<T> shared) {
		std::lock_guard<std::mutex> lock(m_mutex);
		try {
			m_map.at(key);
		} catch(std::out_of_range &e) {
			m_map[key] = shared;
			return true;
		}
		return false;
	}
	
	std::shared_ptr<T> pop (std::string key) {
		std::lock_guard<std::mutex> lock(m_mutex);
		try {
			m_map.at(key);
		} catch(std::out_of_range &e) {
			return nullptr;
		}
		auto r = m_map[key];
		m_map.erase(key);
		return r;
	}

	size_t size (void) {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_map.size();
	}

private:
	std::unordered_map<std::string, std::shared_ptr<T>> m_map;
	std::mutex m_mutex;
};
#endif // _SHARED_H_

#if 0
// test code cpp
int main (void) {
	class CTest {
	public:
		CTest (void) {std::cout << "ctor" << std::endl;}
		virtual ~CTest (void) {std::cout << "dtor" << std::endl;}
		void put (void) {std::cout << "test" << std::endl;}
	};

	CShared<> _shared;
	std::cout << _shared.size() << std::endl;

	{
		auto a = std::make_shared<CTest>();
		_shared.push("a", a);

		auto b = std::shared_ptr<int[]>(new int [2], std::default_delete<int []>());
		b[0] = 100;
		b[1] = 101;
		_shared.push("b", b);

		bool r = _shared.push("b", b);
		if (!r) {
			std::cout << "check duplicated" << std::endl;
		}

		std::cout << _shared.size() << std::endl;
	}

	{
		auto r = _shared.pop("null");
		if (!r) {
			std::cout << "check null" << std::endl;
		}
	}

	{
		auto r = _shared.pop("a");
		auto rr = std::static_pointer_cast<CTest>(r);
		rr->put();
	}

	std::cout << _shared.size() << std::endl;

	{
		auto r = _shared.pop("b");
		auto rr = std::static_pointer_cast<int[]>(r);
		std::cout << rr[0] << "," << rr[1] << std::endl;
	}

	std::cout << _shared.size() << std::endl;
	return 0;
}
#endif
