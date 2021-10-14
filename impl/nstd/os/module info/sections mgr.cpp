#include "sections mgr.h"

#include "nstd/runtime_assert_fwd.h"
#include "nstd/os/module info.h"

#include NSTD_OS_MODULE_INFO_DATA_CACHE_INCLUDE

#include <Windows.h>

#include <mutex>

using namespace nstd::os;

struct sections_mgr::storage_type : NSTD_OS_MODULE_INFO_DATA_CACHE<std::string, section_info>
{
	storage_type(const storage_type&)            = delete;
	storage_type& operator=(const storage_type&) = delete;

	storage_type(storage_type&&)            = default;
	storage_type& operator=(storage_type&&) = default;

	storage_type() = default;
};

sections_mgr::sections_mgr()
{
	storage_ = std::make_unique<storage_type>( );
}

sections_mgr::~sections_mgr()                                  = default;
sections_mgr::sections_mgr(sections_mgr&&) noexcept            = default;
sections_mgr& sections_mgr::operator=(sections_mgr&&) noexcept = default;

const section_info& sections_mgr::at(const std::string_view& name) const
{
	if (storage_->empty( ))
	{
		const auto lock = std::scoped_lock<const root_class_getter>(*this);

		//check can another thread do all the work
		if (storage_->empty( ))
		{
			storage_type temp_storage;
			const auto root = this->root_class( );

			const auto nt           = root->NT( );
			const auto base_address = root->base( );

			const auto number_of_sections = nt->FileHeader.NumberOfSections;
			temp_storage.reserve(number_of_sections);

			const auto section_header      = IMAGE_FIRST_SECTION(nt);
			const auto last_section_header = section_header + number_of_sections;

			for (auto header = section_header; header != last_section_header; ++header)
			{
				auto& raw_name = header->Name;
				auto info_name = std::string(raw_name, std::ranges::find(raw_name, '\0'));

				section_info info;
				info.block = {base_address + header->VirtualAddress, header->SizeOfRawData};
				info.data  = header;

				temp_storage.emplace(std::move(info_name), std::move(info));
			}
			*storage_ = std::move(temp_storage);
		}
	}

	const auto el = storage_->find(std::string(name));
	runtime_assert(el != storage_->end( ));
	return el->second;
}
