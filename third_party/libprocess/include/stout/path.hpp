#ifndef __STOUT_PATH_HPP__
#define __STOUT_PATH_HPP__

#include <string>
#include <vector>

#include "strings.hpp"

namespace path {

inline std::string join(const std::string& path1, const std::string& path2)
{
  return
    strings::remove(path1, "/", strings::SUFFIX) + "/" +
    strings::remove(path2, "/", strings::PREFIX);
}


inline std::string join(const std::vector<std::string>& paths)
{
  if (paths.empty()) {
    return "";
  }

  std::string result = paths[0];
  for (size_t i = 1; i < paths.size(); ++i) {
    result = join(result, paths[i]);
  }
  return result;
}

} // namespace path {

#endif // __STOUT_PATH_HPP__
