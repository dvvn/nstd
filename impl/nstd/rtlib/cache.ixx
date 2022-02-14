module;

#include "cache_includes.h"

export module nstd.rtlib:cache;

export namespace nstd::rtlib
{
	template <typename KeyTr, typename Key>
	concept transparent_type = std::equality_comparable_with<Key, KeyTr> && std::constructible_from<Key, KeyTr>;

	template <typename T, typename ...Args>
	concept have_emplace_hint = requires(T map, typename T::const_iterator itr, typename T::key_type && key, typename T::value_type && val)
	{
		map.emplace_hint(itr, std::forward<T::key_type>(key), std::forward<T::value_type>(val));
	};
	template <typename T, typename Key = typename T::key_type>
	concept have_array_access = requires(const T & map, const Key & key)
	{
		map[key];
	};
	template <typename T, typename Key = typename T::key_type>
	concept have_transparent_find = requires(const T & map, const Key & key)
	{
		map.find(key);
	};
	template <typename T, typename Key = typename T::key_type>
	concept have_transparent_at = requires(const T & map, const Key & key)
	{
		map.at(key);
	};

	template <typename T>
	concept have_hasher = requires
	{
		typename T::hasher;
	};

	template <typename T, typename ...Args>
	typename T::iterator emplace_hint_helper(T& map, typename T::const_iterator hint, Args&&...args)
	{
		if constexpr (have_emplace_hint<T>)
			return map.emplace_hint(hint, std::forward<Args>(args)...);
		else
		{
			auto&& [itr, added] = map.emplace(std::forward<Args>(args)...);
			return itr;
		}
	};

	inline constexpr bool _Ignore_find_errors =
#ifdef _DEBUG
		false
#else
		true
#endif
		;

	template <bool IgnoreErrors = _Ignore_find_errors
		, typename T
		, typename TRaw = std::remove_cvref_t<T>
		, typename KeySelf = typename TRaw::key_type //
		, std::equality_comparable_with<KeySelf> Key>
		auto find_helper(T& map, const Key& key)
	{
		if constexpr (have_transparent_find<TRaw, Key>)
		{
			auto found = map.find(key);
			if constexpr (!IgnoreErrors)
			{
				if (found == map.end( ))
					throw std::out_of_range(__FUNCSIG__": key not found");
			}
			return found;
		}
		else if constexpr (std::is_trivially_constructible_v<KeySelf, Key> || (have_hasher<TRaw> && std::is_constructible_v<KeySelf, Key>))
		{
			return find_helper<IgnoreErrors>(map, KeySelf(key));
		}
		else
		{
			const auto end = map.end( );
			for (auto itr = map.begin( ); itr != end; ++itr)
			{
				if (*itr == key)
					return itr;
			}
			if constexpr (!IgnoreErrors)
				throw std::out_of_range(__FUNCSIG__": key not found");
			return end;
		}
	}

#define NSTD_RTLIB_CACHE_HEAD\
	template <typename KeyType, typename DataType, typename MutexType, bool CreateOnce>
#define NSTD_RTLIB_CACHE_HEAD_ARGS\
	KeyType,DataType,MutexType,CreateOnce

	NSTD_RTLIB_CACHE_HEAD
		class basic_cache
	{
	public:
		using key_type = KeyType;
		using mapped_type = DataType;

		using mutex_type = MutexType;

	protected:
		using create_result = std::conditional_t<CreateOnce, void, mapped_type>;
		virtual create_result create(const key_type& entry/*, CreateArgs ...args*/) = 0;

	private:
		using cache_type = nstd::unordered_map<key_type, mapped_type>;
		using mutex_controller = std::scoped_lock<mutex_type>;

	public:

		/*bool empty( ) const
		{
			return cache_.empty( );
		}

		bool size( ) const
		{
			return cache_.size( );
		}

		auto begin( )const
		{
			return cache_.begin( );
		}

		auto end( )const
		{
			return cache_.end( );
		}*/

		struct at_default_callback
		{
			void operator()(mapped_type* value, bool created)const { }
		};

		template<typename K, typename Callback = at_default_callback>
		mapped_type& at(K&& key, Callback callback = {})
		{
			const auto lock = mutex_controller(mtx_);

			decltype(auto) key_find = [&]( )-> decltype(auto)
			{
				if constexpr (have_transparent_find<cache_type, K>)
					return std::forward<K>(key);
				else
					return KeyType(std::forward<K>(key));
			}();

			bool created;
			mapped_type* ret;

			auto found = find_helper<true>(cache_, key_find);
			if (found != cache_.end( ))
			{
				created = false;
				ret = std::addressof(found->second);
			}
			else
			{
				created = true;
				decltype(auto) key_create = [&]( )-> decltype(auto)
				{
					if constexpr (!have_transparent_find<cache_type, K>) //already converted to KeyType
					{
						//rvalue for emplace
						return std::move(key_find);
					}
					else if constexpr (std::same_as<KeyType, std::remove_cvref_t<K>>) //not converted, already in correct type
					{
						return std::forward<K>(key);
					}
					else //type is incorrect, construct KeyType
					{
						return KeyType(std::forward<K>(key));
					}
				};

				if constexpr (!CreateOnce)
				{
					auto key_create_obj = key_create( );
					auto item = this->create(key_create_obj);
					auto placed = emplace_hint_helper(cache_, cache_.end( ), std::move(key_create_obj), std::move(item));
					ret = std::addressof(placed->second);
				}
				else
				{
					if (!cache_.empty( ))//Cache already filled, but key not found
					{
						ret = nullptr;
					}
					else
					{
						this->create(key_create( ));
						found = find_helper(cache_, key_find);
#ifdef _DEBUG
						if (found == cache_.end( ))
							throw std::logic_error("Key created but not found");
#endif
						ret = std::addressof(found->second);
					}
				}
			}

			std::invoke(callback, ret, created);
			return *ret;
		}

		template<typename K>
		mapped_type* find(const K& key)
		{
			const auto lock = mutex_controller(mtx_);

			auto found = find_helper<true>(cache_, key);
			if (found == cache_.end( ))
				return nullptr;
			return std::addressof(found->second);
		}

		template<typename K>
		const mapped_type* find(const K& key)const
		{
			return const_cast<basic_cache*>(this)->find(key);
		}

		template<typename K>
		const mapped_type& operator[](const K& key) const
		{
			const auto lock = mutex_controller(mtx_);
			if constexpr (!_Ignore_find_errors)
			{
				if constexpr (have_transparent_at<cache_type, K>)
					return cache_.at(key);
				else if constexpr (have_array_access<cache_type, K>)
					return cache_[key];
			}
			return find_helper(cache_, key)->second;
		}

	protected:
		void reserve(size_t size)
		{
			cache_.reserve(size);
		}

		template <typename K, typename D>
		mapped_type& emplace(K&& key, D&& data)
		{
			auto itr = emplace_hint_helper(cache_, cache_.end( ), key_type(std::forward<K>(key)), mapped_type(std::forward<D>(data)));
			return itr->second;
		}

	public:
		basic_cache( ) = default;
		basic_cache(basic_cache&& other) noexcept
			:cache_(std::move(other.cache_))
		{
		}
		basic_cache& operator=(basic_cache&& other)noexcept
		{
			cache_ = std::move(other.cache_);
			return *this;
		}

		void swap(basic_cache& r)
		{
			using nstd::swap;
			swap(this->cache_, r.cache_);
		}

	private:
		cache_type cache_;
		mutable mutex_type mtx_;
	};

	NSTD_RTLIB_CACHE_HEAD
		void swap(basic_cache<NSTD_RTLIB_CACHE_HEAD_ARGS>& l, basic_cache<NSTD_RTLIB_CACHE_HEAD_ARGS>& r)
	{
		l.swap(r);
	}

	struct info;
	struct root_getter
	{
		virtual ~root_getter( ) = default;

		virtual info* root_class( ) = 0;
		virtual const info* root_class( ) const = 0;
	};

	template<typename DataType, bool CreateOnce>
	struct cache :basic_cache<std::string, DataType, std::recursive_mutex, CreateOnce>, protected virtual root_getter
	{
	};
}
