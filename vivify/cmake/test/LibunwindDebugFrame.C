/*!
 * \file
 * Test if libunwind was built with --enable-debug-frame
 *
 * \author Andrew C., Morgan Stanley
 */

#include <libunwind.h>

extern "C" int UNW_OBJ(dwarf_find_debug_frame)(
  int found_, unw_dyn_info_t* di_, unw_word_t ip_, unw_word_t segbase_, const char* objName_,
  unw_word_t start_, unw_word_t end_
);
#define dwarf_find_debug_frame UNW_OBJ(dwarf_find_debug_frame)

int main (int /*argc*/, char** /*argv_*/)
{
  unw_dyn_info_t l_di{};
  dwarf_find_debug_frame(0, &l_di, 0u, 0u, nullptr, 0u, 0u);
  return 0;
}
