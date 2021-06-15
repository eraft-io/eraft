#include <memory>
#include <stdint.h>
#include <string>
#include <vector>


namespace kvserver
{

class File
{

public:

    ~File() noexcept;
    File(const File&) = delete;
    File(File&& other) noexcept;
    File& operator=(const File&) = delete;
    File& operator=(File&& other) noexcept;

public:

    static bool IsAbsolutePath(const std::string& path);

    static std::string GetExeImagePath();

    static std::string GetExeParentDirectory();

    static void ListDirectory(const std::string& directory, std::vector< std::string >& list);

    static bool CreateDirectory(const std::string& directory);

    static bool DeleteDirectory(const std::string& directory);

};


} // namespace kvserver
