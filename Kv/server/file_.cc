#include <Kv/file.h>

#include <regex>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

namespace kvserver
{

bool File::IsAbsolutePath(const std::string &path) {
    static std::regex absolutePathRegex("[~/].*");
    return std::regex_match(path, absolutePathRegex);
}

bool File::DeleteDirectory(const std::string &directory) {
    std::string directoryWithSeparator(directory);
    if (
        (directoryWithSeparator.length() > 0)
        && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
    ) {
        directoryWithSeparator += '/';
    }
    DIR* dir = opendir(directory.c_str());
    if (dir != NULL) {
        struct dirent entry;
        struct dirent* entryBack;
        while (true) {
            if (readdir_r(dir, &entry, &entryBack)) {
                break;
            }
            if (entryBack == NULL) {
                break;
            }
            std::string name(entry.d_name);
            if ((name == ".") || (name == "..")) {
                continue;
            }
            std::string filePath(directoryWithSeparator);
            filePath += entry.d_name;
            if (entry.d_type == DT_DIR) {
                if (!DeleteDirectory(filePath.c_str())) {
                    return false;
                }
            } else {
                if (unlink(filePath.c_str()) != 0) {
                    return false;
                }
            }
        }
        (void)closedir(dir);
        return (rmdir(directory.c_str()) == 0);
    }
    return true;
}


} // namespace kvserver
