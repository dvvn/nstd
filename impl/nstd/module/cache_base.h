#pragma once

#include <concepts>
#include <utility>
// ReSharper disable CppUnusedIncludeDirective
#include <memory>
#include <string>
// ReSharper restore CppUnusedIncludeDirective

namespace nstd::module
{
	class info;

	namespace detail
	{
		template <typename KeyTr, typename Key>
		concept transparent_type = std::equality_comparable_with<Key, KeyTr> && std::constructible_from<Key, KeyTr>;

		template <typename T>
		using ref_or_copy = std::conditional_t<std::is_trivially_copy_assignable_v<T> && std::is_trivially_destructible_v<T> && sizeof(T) <= sizeof(size_t), T, const T&>;

		template <typename KeyType
				, typename DataType
				, transparent_type<KeyType> KeyTypeTransparent
				, typename ...CreateArgs>
		class __declspec(novtable) cache
		{
		public:
			virtual ~cache( ) = default;

			using key_entry = ref_or_copy<KeyType>;
			using key_entry_tr = ref_or_copy<KeyTypeTransparent>;
			using return_value = ref_or_copy<DataType>;

			using key_type = KeyType;
			using mapped_type = DataType;

			//lock & generate data or access if exists
			virtual return_value at(key_entry entry, CreateArgs ...create_args) = 0;
			virtual return_value at(key_entry_tr entry, CreateArgs ...create_args) = 0;

			virtual mapped_type* find(key_entry entry) = 0;
			virtual mapped_type* find(key_entry_tr entry) = 0;
			const mapped_type* find(key_entry entry) const { return const_cast<cache*>(this)->find(entry); }
			const mapped_type* find(key_entry_tr entry) const { return const_cast<cache*>(this)->find(entry); }

			virtual return_value operator[](key_entry entry) const = 0;
			virtual return_value operator[](key_entry_tr entry) const = 0;

		protected:
			using create_result = std::pair<mapped_type, bool>;
			virtual create_result create(const key_type& entry, CreateArgs ...args) = 0;
			virtual bool empty( ) const = 0;
			virtual void reserve(size_t size) = 0;
			virtual mapped_type& emplace(key_type&& key, mapped_type&& data) = 0;
		};

		struct root_getter
		{
			virtual ~root_getter( ) = default;

			virtual info* root_class( ) = 0;
			virtual const info* root_class( ) const = 0;
		};
	}
}

#define NSTD_OS_MODULE_INFO_CACHE_IMPL_H0(_OWNER_,_TYPE_) \
	class _OWNER_ : public detail::cache<std::string, _TYPE_, std::string_view>, protected virtual detail::root_getter\
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

#define NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP0(_OWNER_,_TYPE_) \
	struct _OWNER_##_impl : nstd::module::detail::cache_impl<std::string, _TYPE_, std::string_view, std::recursive_mutex, info*> {\
	protected:\
		create_result create(const key_type& entry, info* info_ptr) override;\
	};\
	struct _OWNER_::impl : _OWNER_##_impl{ friend class _OWNER_; };\
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
	auto _OWNER_##_impl::create(const key_type& entry, info* info_ptr) -> create_result

#define NSTD_OS_MODULE_INFO_CACHE_IMPL_DATA(_NAME_) struct _NAME_##_t
#define NSTD_OS_MODULE_INFO_CACHE_IMPL_H(_NAME_) NSTD_OS_MODULE_INFO_CACHE_IMPL_H0(_NAME_##s,_NAME_##_t)
#define NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP(_NAME_) NSTD_OS_MODULE_INFO_CACHE_IMPL_CPP0(_NAME_##s,_NAME_##_t)
