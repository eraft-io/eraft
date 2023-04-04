#pragma once
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;
class DirectoryTool
{
public:
    static bool IsDir(const std::string dirpath);
    static bool MkDir(const std::string dirpath);
    static int StaticDirSize(const std::string dirpath);
    static std::vector<std::string> ListDirFiles(const std::string dirpath);
    static void DeleteDir(const std::string dirpath);  
    DirectoryTool(/* args */);
    ~DirectoryTool();
};

