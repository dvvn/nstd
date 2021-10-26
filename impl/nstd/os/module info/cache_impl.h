#pragma once

#include "cache_base.h"

#include <mutex>

#if __has_include(<robin_hood.h>)
#define NSTD_OS_MODULE_INFO_DATA_CACHE robin_hood::unordered_map
#define NSTD_OS_MODULE_INFO_DATA_CACHE_STD 0
#include <robin_hood.h>
#else
#define NSTD_OS_MODULE_INFO_DATA_CACHE std::unordered_map
#define NSTD_OS_MODULE_INFO_DATA_CACHE_STD 1
#include <unordered_map>
#endif

namespace nstd::os
{
	namespace detail
	{
		template <typename T, typename ...Args>
		concept have_emplace_hint = requires(T map, typename T::const_iterator itr, typename T::key_type&& key, typename T::value_type&& val)
		{
			map.emplace_hint(itr, std::forward<T::key_type>(key), std::forward<T::value_type>(val));
		};
		template <typename T, typename Key = typename T::key_type>
		concept have_array_access = requires(const T& map, const Key& key)
		{
			map[key];
		};
		template <typename T, typename Key = typename T::key_type>
		concept have_transparent_find = requires(const T& map, const Key& key)
		{
			map.find(key);
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
		}

		template <typename T, typename TRaw = std::remove_cvref_t<T>, std::equality_comparable_with<typename TRaw::key_type> Key>
		auto find_helper(T&& map, const Key& key)
		{
			if constexpr (have_transparent_find<TRaw, Key>)
				return (map.find(key));
			else if constexpr (std::constructible_from<typename TRaw::key_type, decltype(key)>)
				return map.find(typename TRaw::key_type(key));
			else
				static_assert(std::_Always_false<Key>, __FUNCTION__": unsupported key_type!");
		}

		template <typename T, std::equality_comparable_with<typename T::key_type> Key>
		const typename T::mapped_type& const_access_helper(const T& map, const Key& key)
		{
			if constexpr (have_array_access<T, Key>)
				return map[key];
			else
				return find_helper(map, key)->second;
		}

		//---
	}

	template <typename KeyType
			, typename DataType
			, typename KeyTypeTransparent
			, typename MutexType = std::mutex //
			, typename ...CreateArgs>
	class module_info_cache_impl : public module_info_cache<KeyType, DataType, KeyTypeTransparent, CreateArgs...>
	{
#ifdef _MUTEX_
		using lock_unlock = std::scoped_lock<MutexType>;
#else
		class _NODISCARD lock_unlock
		{
		public:
			explicit lock_unlock(MutexType& mtx)
				: mutex_(mtx)
			{
				mutex_.lock();
			}

			~lock_unlock() noexcept
			{
				mutex_.unlock();
			}

			lock_unlock(const lock_unlock&) = delete;
			lock_unlock& operator=(const lock_unlock&) = delete;

		private:
			MutexType& mutex_;
		};
#endif

	public:
		using Access = module_info_cache<KeyType, DataType, KeyTypeTransparent, CreateArgs...>;

		using key_entry = typename Access::key_entry;
		using key_entry_tr = typename Access::key_entry_tr;
		using return_value = typename Access::return_value;

		using create_result = typename Access::create_result;

		using key_type = typename Access::key_type;
		using mapped_type = typename Access::mapped_type;

		using mutex_type = MutexType;

	private:
		using cache_type = NSTD_OS_MODULE_INFO_DATA_CACHE<key_type, mapped_type>;

		template <typename T>
		return_value at_universal(T&& entry, CreateArgs ...create_args)
		{
			const auto lock = lock_unlock(locker_);

			decltype(auto) entry_find = [&]( )-> decltype(auto)
			{
				if constexpr (detail::have_transparent_find<cache_type, T>)
					return std::forward<T>(entry);
				else
					return KeyType(std::forward<T>(entry));
			}( );

			auto found = detail::find_helper(cache_, entry_find);
			if (found != cache_.end( ))
				return found->second;

			decltype(auto) entry_create = [&]( )-> decltype(auto)
			{
				if constexpr (!detail::have_transparent_find<cache_type, T>) //already converted to KeyType
				{
					//rvalue for emplace
					return std::move(entry_find);
				}
				else if constexpr (std::same_as<KeyType, std::remove_cvref_t<T>>) //not converted, already in correct type
				{
					return std::forward<T>(entry);
				}
				else //type is incorrect, construct KeyType
				{
					return KeyType(std::forward<T>(entry));
				}
			}( );

			auto [mapped,created] = this->create(entry_create, create_args...);
			if (!created)
				return detail::find_helper(cache_, entry_create)->second;

			auto itr = detail::emplace_hint_helper(cache_, cache_.end( ), std::forward<decltype(entry_create)>(entry_create), std::move(mapped));
			return itr->second;
		}

		template <typename T>
		return_value access_universal(const T& entry) const
		{
			const auto lock = lock_unlock(locker_);
			return detail::const_access_helper(cache_, entry);
		}

		template <typename T>
		mapped_type* find_universal(const T& entry)
		{
			const auto lock = lock_unlock(locker_);
			return std::addressof(detail::find_helper(cache_, entry)->second);
		}

		template <typename T>
		const mapped_type* find_universal(const T& entry) const
		{
			return std::_Const_cast(this)->find_universal(entry);
		}

	public:
		return_value at(key_entry entry, CreateArgs ...create_args) final { return this->at_universal(entry, create_args...); }
		return_value at(key_entry_tr entry, CreateArgs ...create_args) final { return this->at_universal(entry, create_args...); }

		mapped_type* find(key_entry entry) final { return this->find_universal(entry); }
		mapped_type* find(key_entry_tr entry) final { return this->find_universal(entry); }

		return_value operator[](key_entry entry) const final { return this->access_universal(entry); }
		return_value operator[](key_entry_tr entry) const final { return this->access_universal(entry); }

	protected:
		bool empty( ) const final { return cache_.empty( ); }
		void reserve(size_t size) final { cache_.reserve(size); }

		mapped_type& emplace(key_type&& key, mapped_type&& data) final
		{
			auto itr = detail::emplace_hint_helper(cache_, cache_.end( ), std::move(key), std::move(data));
			return itr->second;
		}

	private:
		cache_type cache_;
		mutable mutex_type locker_;
	};
}
