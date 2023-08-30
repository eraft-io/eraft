/**
 * @file sequential_file_writer.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <fstream>
#include <string>

class SequentialFileWriter {
public:
    
    SequentialFileWriter();
    SequentialFileWriter(SequentialFileWriter&&);
    SequentialFileWriter& operator=(SequentialFileWriter&&);

    // Open the file at the relative path 'name' for writing. On errors throw std::system_error
    void OpenIfNecessary(const std::string& name);

    // Write data from a string. On errors throws an exception drived from std::system_error
    // This method may take ownership of the string. Hence no assumption may be made about
    // the data it contains after it returns.
    void Write(std::string& data);

    bool NoSpaceLeft() const
    {
        return m_no_space;
    }

private:
    std::string m_name;
    std::ofstream m_ofs;
    bool m_no_space;

    // void RaiseError [[noreturn]] (const std::string action_attempted, const std::system_error& ex);
};
