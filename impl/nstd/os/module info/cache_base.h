#pragma once

#include <concepts>
#include <utility>
// ReSharper disable CppUnusedIncludeDirective
#include <memory>
#include <string>
// ReSharper restore CppUnusedIncludeDirective

#if __has_include(<robin_hood.h>)
#define NSTD_OS_MODULE_INFO_DATA_INCLUDE <robin_hood.h>
#define NSTD_OS_MODULE_INFO_DATA_CACHE robin_hood::unordered_map
#define NSTD_OS_MODULE_INFO_DATA_CACHE_STD 0
#else
#define NSTD_OS_MODULE_INFO_DATA_INCLUDE <unordered_map>
#define NSTD_OS_MODULE_INFO_DATA_CACHE std::unordered_map
#define NSTD_OS_MODULE_INFO_DATA_CACHE_STD 1
#endif

namespace nstd::os
{
	namespace detail
	{
		template <typename T>
		using ref_or_copy = std::conditional_t<std::is_trivially_copy_assignable_v<T> && std::is_trivially_destructible_v<T>, T, const T&>;

		template <typename KeyTr, typename Key>
		concept transparent_key = std::equality_comparable_with<Key, KeyTr> && std::constructible_from<Key, KeyTr>;
	}

	template <typename KeyType
			, typename DataType
			, detail::transparent_key<KeyType> KeyTypeTransparent
			, typename ...CreateArgs>
	class __declspec(novtable) module_info_cache
	{
	public:
		virtual ~module_info_cache( ) = default;

		using key_entry = detail::ref_or_copy<KeyType>;
		using key_entry_tr = detail::ref_or_copy<KeyTypeTransparent>;
		using return_value = detail::ref_or_copy<DataType>;

		using key_type = KeyType;
		using mapped_type = DataType;

		//lock & generate data or access if exists
		virtual return_value at(key_entry entry, CreateArgs ...create_args) =0;
		virtual return_value at(key_entry_tr entry, CreateArgs ...create_args) =0;

		virtual mapped_type* find(key_entry entry) =0;
		virtual mapped_type* find(key_entry_tr entry) =0;
		const mapped_type* find(key_entry entry) const { return const_cast<module_info_cache*>(this)->find(entry); }
		const mapped_type* find(key_entry_tr entry) const { return const_cast<module_info_cache*>(this)->find(entry); }

		virtual return_value operator[](key_entry entry) const =0;
		virtual return_value operator[](key_entry_tr entry) const =0;

	protected:
		using create_result = std::pair<mapped_type, bool>;
		virtual create_result create(const key_type& entry, CreateArgs ...args) =0;
		virtual bool empty( ) const =0;
		virtual void reserve(size_t size) =0;
		virtual mapped_type& emplace(key_type&& key, mapped_type&& data) =0;
	};

	class module_info;

	class module_info_getter
	{
	protected:
		virtual ~module_info_getter( ) = default;

		virtual module_info* root_class( ) =0;
		virtual const module_info* root_class( ) const =0;
	};

#define NSTD_OS_MODULE_INFO_CACHE_IMPL_H(_OWNER_,_TYPE_) \
	struct _OWNER_ : module_info_cache<std::string, _TYPE_, std::string_view>, virtual module_info_getter\
	{\
	public:\
		_OWNER_();\
		~_OWNER_() override;\
		_OWNER_(_OWNER_&&) noexcept;\
		_OWNER_& operator=(_OWNER_&&) noexcept;\
		return_value at(key_entry entry) final;\
		return_value at(key_entry_tr entry) final;\
		mapped_type* find(key_entry entry) final;\
		mapped_type* find(key_entry_tr entry) final;\
		return_value operator[](key_entry entry) const final;\
		return_value operator[](key_entry_tr entry) const final;\
	protected:\
		create_result create(const key_type& entry) final;\
		bool empty( ) const final;\
		void reserve(size_t size);\
		mapped_type& emplace(key_type&& key, mapped_type&& data) final;\
	private:\
		struct impl;\
		std::unique_ptr<impl> impl_;\
	}

#define NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP(_OWNER_,_TYPE_) \
	struct _OWNER_##_impl : module_info_cache_impl<std::string, _TYPE_, std::string_view, std::recursive_mutex, module_info*> {\
	protected:\
		create_result create(const key_type& entry, module_info* module_info_ptr) override;\
	};\
	struct _OWNER_::impl : _OWNER_##_impl{ friend struct _OWNER_; };\
	_OWNER_::_OWNER_() { impl_ = std::make_unique<impl>();}\
	_OWNER_::~_OWNER_() = default;\
	_OWNER_::_OWNER_(_OWNER_&&) noexcept = default;\
	_OWNER_& _OWNER_::operator=(_OWNER_&&) noexcept = default;\
	auto _OWNER_::at(key_entry entry) -> return_value {return impl_->at(entry, this->root_class());}\
	auto _OWNER_::at(key_entry_tr entry) -> return_value {return impl_->at(entry, this->root_class());}\
	auto _OWNER_::find(key_entry entry) -> mapped_type* {return impl_->find(entry);}\
	auto _OWNER_::find(key_entry_tr entry) -> mapped_type* {return impl_->find(entry);}\
	auto _OWNER_::operator[](key_entry entry) const -> return_value {return impl_->operator[](entry);}\
	auto _OWNER_::operator[](key_entry_tr entry) const -> return_value {return impl_->operator[](entry);}\
	auto _OWNER_::create(const key_type& entry) -> create_result {return impl_->create(entry, this->root_class());}\
	bool _OWNER_::empty() const {return impl_->empty();}\
	void _OWNER_::reserve(size_t size) {impl_->reserve(size);}\
	auto _OWNER_::emplace(key_type&& key, mapped_type&& data) -> mapped_type& {return impl_->emplace(std::move(key), std::move(data));}\
	\
	auto _OWNER_##_impl::create(const key_type& entry, module_info* module_info_ptr) -> create_result
}
