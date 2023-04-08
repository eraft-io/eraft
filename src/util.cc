#include "util.h"

bool DirectoryTool::IsDir(const std::string dirpath) {
  return fs::is_directory(dirpath);
};

bool DirectoryTool::MkDir(const std::string dirpath) {
  return fs::create_directories(dirpath);
}

/*
return -1 if dirpath not exist
return size of all files in directory in bytes
*/
int DirectoryTool::StaticDirSize(const std::string dirpath) {
  if (!fs::is_directory(dirpath)) {
    return -1;
  }
  int allfilessize = 0;
  for (auto const& dir_entry : fs::recursive_directory_iterator(dirpath)) {
    if (fs::is_regular_file(dir_entry)) {
      allfilessize += dir_entry.file_size();
    }
  }
  return allfilessize;
}

/*
Recursive List files in directory
*/
std::vector<std::string> DirectoryTool::ListDirFiles(
    const std::string dirpath) {
  std::vector<std::string> filenames;
  if (!fs::is_directory(dirpath)) {
    return filenames;
  }
  for (auto const& dir_entry : fs::recursive_directory_iterator(dirpath)) {
    filenames.push_back(dir_entry.path());
  }
  return filenames;
}

/*
Recursive delete directory and all files in the directory
*/
void DirectoryTool::DeleteDir(const std::string dirpath) {
  fs::remove_all(dirpath);
}

DirectoryTool::DirectoryTool(/* args */) {}

DirectoryTool::~DirectoryTool() {}
