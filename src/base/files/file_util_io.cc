#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"

namespace base {

bool ReadFileToStringWithMaxSize(const FilePath& path,
                                 std::string* contents,
                                 size_t max_size) {
  if (contents)
    contents->clear();
  if (path.ReferencesParent())
    return false;

  auto OpenFile = [] (const FilePath& filename, const char* mode) {
    ThreadRestrictions::AssertIOAllowed();
    FILE* result = NULL;
    do {
      result = fopen(filename.value().c_str(), mode);
    } while (!result && errno == EINTR);
    return result;
  };

  FILE* file = OpenFile(path, "rb");
  if (!file) {
    return false;
  }

  const size_t kBufferSize = 1 << 16;
  std::unique_ptr<char[]> buf(new char[kBufferSize]);
  size_t len;
  size_t size = 0;
  bool read_status = true;

  // Many files supplied in |path| have incorrect size (proc files etc).
  // Hence, the file is read sequentially as opposed to a one-shot read.
  while ((len = fread(buf.get(), 1, kBufferSize, file)) > 0) {
    if (contents)
      contents->append(buf.get(), std::min(len, max_size - size));

    if ((max_size - size) < len) {
      read_status = false;
      break;
    }

    size += len;
  }
  read_status = read_status && !ferror(file);
  fclose(file);

  return read_status;
}

} // base
