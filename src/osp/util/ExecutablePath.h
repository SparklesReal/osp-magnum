#pragma once
#include <string>

namespace osp::filefunctions
{
std::string_view get_exe_dir();
std::string_view const s_exe_dir = get_exe_dir(); // get_exe_dir is only called once
}