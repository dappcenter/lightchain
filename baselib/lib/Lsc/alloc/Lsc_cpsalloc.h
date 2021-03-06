

#ifndef  __BSL_CPSALLOC_H_
#define  __BSL_CPSALLOC_H_

#include <Lsc/containers/deque/Lsc_deque.h>

namespace Lsc
{
/**
*
*  This is precisely the allocator defined in the C++ Standard.
*    - all allocation calls malloc
*    - all deallocation calls free
*/

template < class _Alloc >
class Lsc_cpsalloc
{
	static const unsigned long DATAQSIZE = 1024UL * 16UL;
plclic:
	typedef _Alloc _Base;
	typedef typename _Base::pool_type pool_type;
	typedef typename _Base::size_type size_type;
	typedef typename _Base::difference_type difference_type;
	typedef unsigned int pointer;
	typedef const unsigned int const_pointer;
	typedef typename _Base::reference reference;
	typedef typename _Base::const_reference const_reference;
	typedef typename _Base::value_type value_type;	
	typedef Lsc_cpsalloc<_Alloc> _Self;
	
	template <typename _Tp1>
	struct rebind {
	private:
		typedef typename _Base::template rebind<_Tp1>::other other_alloc;
	plclic:
		typedef Lsc_cpsalloc<other_alloc> other;
	};

	static const bool recycle_space;// = true;	//在alloc析构的时候,是否释放空间
	static const bool thread_safe;// = false;	//空间分配器是否线程安全

	union data_t
	{
		data_t *next;
		char data[sizeof(value_type)];
	};

	Lsc::deque<pointer> _free;
	Lsc::deque<data_t, DATAQSIZE> _deque;

	Lsc_cpsalloc()  {
		create();
	}

	Lsc_cpsalloc(const Lsc_cpsalloc &)  {
		create();
	}

	template < typename _Tp1 >
	Lsc_cpsalloc(const Lsc_cpsalloc < _Tp1 > &)  {
		create();
	}

	~Lsc_cpsalloc()  {
		destroy();
	}

#if 0
	pointer address(reference __x) const {
		return &__x;
	}

	const_pointer address(const_reference __x) const {
		return &__x;
	}
#endif

	//NB:__n is permitted to be 0. The C++ standard says nothing
	// about what the return value is when __n == 0.
	pointer allocate(size_type __n, const void * = 0) {
		if (__n > 1) return 0;
		if (_free.size() > 0) {
			pointer p = _free.front();
			_free.pop_front();
			return p;
		}
		_deque.push_back(data_t());
		return _deque.size();
	}

	//__p is not permitted to be a null pointer.
	void deallocate(pointer __p, size_type) {
		_free.push_back(__p);
	}

	size_type max_size() const  {
		return size_t(-1) / sizeof(value_type);
	}

	//_GLIBCXX_RESOLVE_LIB_DEFECTS
	//402. wrong new expression in[some_] allocator: :                construct
	void construct(pointer __p, const value_type & __val) {
		::new(getp(__p)) value_type(__val);
	}

	void destroy(pointer __p) {
		getp(__p)->~ value_type();
	}

	int create() {
		return 0;
	}

	int destroy() {
		_free.destroy();
		_deque.destroy();
		return 0;
	}

	void swap (_Self &__self) {
		_free.swap(__self._free);
		_deque.swap(__self._deque);
	}

	value_type * getp(pointer __p) const {
		return (value_type *)(_deque[__p-1].data);
	}
};

template < typename _Tp >
inline bool operator == (const Lsc_cpsalloc < _Tp > &, const Lsc_cpsalloc < _Tp > &) {
	return false;
}

template < typename _Tp, class _Alloc2 > 
inline bool operator == (const Lsc_cpsalloc <_Tp > &, const _Alloc2 &) {
	return false;
}

template < typename _Tp >
inline bool operator != (const Lsc_cpsalloc < _Tp > &, const Lsc_cpsalloc < _Tp > &) {
	return true;
}

template < typename _Tp, class _Alloc2 > 
inline bool operator != (const Lsc_cpsalloc <_Tp > &, const _Alloc2 &) {
	return true;
}

template <class _Alloc>
const bool Lsc_cpsalloc<_Alloc>::recycle_space = true;
template <class _Alloc>
const bool Lsc_cpsalloc<_Alloc>::thread_safe = false;
}

#endif  //__BSL_CPSALLOC_H_

/* vim: set ts=4 sw=4 sts=4 tw=100 noet: */
