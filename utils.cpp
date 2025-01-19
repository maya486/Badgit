#include "utils.h"
#include <iostream>

using namespace std;

void delete_empty_dirs(const filesystem::path &dir) {

  // Iterate through the directory
  for (const auto &entry : filesystem::directory_iterator(dir)) {
    if (filesystem::is_directory(entry)) {
      // Recursively delete empty directories in subdirectories
      delete_empty_dirs(entry.path());
    }
  }

  // If the directory is empty, remove it
  if (filesystem::is_directory(dir) && filesystem::is_empty(dir)) {
    filesystem::remove(dir);
  }
}
