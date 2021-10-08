#include "handle.h"

#include <Windows.h>

using namespace nstd::os;

void handle_closer::operator()(HANDLE ptr) const
{
	CloseHandle(ptr);
}

handle::handle(HANDLE ptr)
	: handle_base(ptr)
{
}
