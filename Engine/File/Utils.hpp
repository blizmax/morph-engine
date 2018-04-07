#pragma once

#include "Engine/Core/common.hpp"
#include "Engine/File/Blob.hpp"
#include "Path.hpp"

class Blob;


Blob fileToBuffer(const char* nameWithFullPath);

namespace fs {
  // environment
  path workingPath();
  path absolute(const path& path, const Path& base = workingPath());
  bool exists(const path& file);
  bool isDirectory(const path& path);
  
  int64 sizeOf(const path& file);
  byte* read(const path& file);
  bool  read(const path& file, void* buffer, int64 size = -1);
}