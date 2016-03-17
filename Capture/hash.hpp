#ifndef IndePlatform_hash_hpp
#define IndePlatform_hash_hpp

#include <numeric>

namespace h3d {
	namespace std {
		inline size_t hase_seq(const unsigned char *_First, size_t _Count)
		{	// FNV-1a hash function for bytes in [_First, _First + _Count)
#if defined(_WIN64)
			static_assert(sizeof(size_t) == 8, "This code is for 64-bit size_t.");
			const size_t _FNV_offset_basis = 14695981039346656037ULL;
			const size_t _FNV_prime = 1099511628211ULL;

#else /* defined(_WIN64) */
			static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
			const size_t _FNV_offset_basis = 2166136261U;
			const size_t _FNV_prime = 16777619U;
#endif /* defined(_WIN64) */

			size_t _Val = _FNV_offset_basis;
			for (size_t _Next = 0; _Next < _Count; ++_Next)
			{	// fold in another byte
				_Val ^= (size_t)_First[_Next];
				_Val *= _FNV_prime;
			}
			return (_Val);
		}

		template<class _Kty>
		struct bitwise_hash
		{	// hash functor for plain old data
			typedef _Kty argument_type;
			typedef size_t result_type;

			size_t operator()(const _Kty& _Keyval) const
			{	// hash _Keyval to size_t value by pseudorandomizing transform
				return (hase_seq((const unsigned char *)&_Keyval, sizeof(_Kty)));
			}
		};

		template<class _Kty>
		struct hash
			: public bitwise_hash<_Kty>
		{	// hash functor for enums
		};

	}

	template<typename _type>
	inline void
		hash_combine(size_t& seed, const _type& val)
	{
		seed ^= std::hash<_type>()(val) + 0x9E3779B9 + (seed << 6) + (seed >> 2);
	}

	template<typename _tIn>
	inline size_t
		hash(size_t seed, _tIn first, _tIn last)
	{
		return ::std::accumulate(first, last, seed,
			[](size_t s, decltype(*first) val) {
			hash_combine(s, val);
			return s;
		});
	}
	template<typename _tIn>
	inline size_t
		hash(_tIn first, _tIn last)
	{
		return hash(0, first, last);
	}


	template< class T, unsigned N >
	inline size_t hash(const T(&x)[N])
	{
		return hash(x, x + N);
	}

	template< class T, unsigned N >
	inline size_t hash(T(&x)[N])
	{
		return hash(x, x + N);
	}
}

#endif