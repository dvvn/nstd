#include "checksum.h"

#include <filesystem>
#include <fstream>

using path = std::filesystem::path;
using namespace nstd::detail;

size_t checksum_impl::operator()(const path& p) const
{
	using namespace std;
	if (exists(p))
	{
		auto ifs = ifstream(p);
		using itr_t = istreambuf_iterator<char>;
		if (!ifs.fail( ))
		{
			const auto tmp = vector(itr_t(ifs), itr_t( ));
			return invoke(*this, span(tmp));
		}
	}
	return 0;
}
