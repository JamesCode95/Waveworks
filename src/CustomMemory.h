#pragma once

#include <sstream>

void* internalMalloc( size_t size );
void internalFree( void* p );
void resetMemoryManagementCallbacksToDefaults();

#if defined(TARGET_PLATFORM_NIXLIKE)
#define __CRTDECL
#endif

template <class T> class myAllocator
{
public:
	typedef size_t            size_type;
	typedef ptrdiff_t         difference_type;
	typedef T*                pointer;
	typedef const T*          const_pointer;
	typedef T&                reference;
	typedef const T&          const_reference;
	typedef T                 value_type;      
	template <class U>        struct rebind { typedef myAllocator<U> other; };
	myAllocator () throw() { }
	myAllocator (const myAllocator&) throw () { }
	template <class U> myAllocator(const myAllocator<U>&) throw() { }
	template <class U> myAllocator& operator=(const myAllocator<U>&) throw() { return *this; }
	~myAllocator () throw() { }
	pointer address(reference value) const { return &value; }
	const_pointer address(const_reference value) const { return &value; }
	size_type max_size() const throw() { return 1000000; }
	void construct(pointer p, const T& value) { new((void*)p) T(value); }
	void destroy(pointer p) { p->~T(); }

	pointer allocate (size_type num, typename myAllocator<T>::const_pointer = 0)
	{
		return (pointer)(internalMalloc(num*sizeof(T)));
	}
	void deallocate(pointer p, size_type n)
	{
		internalFree((void*)p);
	}
};
template <class T, class U> bool operator==(const myAllocator<T>&, const myAllocator<U>&) throw() { return true; }
template <class T, class U> bool operator!=(const myAllocator<T>&, const myAllocator<U>&) throw() { return false; }

typedef std::basic_string< char, std::char_traits<char>, myAllocator< char > > mystring;
typedef std::basic_stringstream< char, std::char_traits<char>, myAllocator< char > > mystringstream;

