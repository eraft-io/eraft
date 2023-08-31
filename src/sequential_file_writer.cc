/**
 * @file sequential_file_writer.cc
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <utility>
#include <stdexcept>
#include <cstdio>
#include <sstream>
#include <sys/errno.h>

#include "sequential_file_writer.h"

SequentialFileWriter::SequentialFileWriter()
    : m_no_space(false)
{
}

SequentialFileWriter::SequentialFileWriter(SequentialFileWriter&&) = default;
SequentialFileWriter& SequentialFileWriter::operator=(SequentialFileWriter&&) = default;

// Currently the implementation is very simple using standard library facilities. More advanced implementations
// allowing for better parallelism are possible, e.g. using aio_write().

void SequentialFileWriter::OpenIfNecessary(const std::string& name)
{
    // FIXME: Sanitise file names. Currently there's nothing preventing the user from giving absolute paths,
    // Paths with .. etc. We should accept simple relative paths only.

    if (m_ofs.is_open()) {
        return;
    }

    using std::ios_base;
    std::ofstream ofs;
    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // TODO: If the given relative path has a directory component, create it.
        ofs.open(name, ios_base::out | ios_base::trunc | ios_base::binary);
    }
    catch (const std::system_error& ex) {
        // RaiseError("opening", ex);
    }

    m_ofs = std::move(ofs);
    m_name = name;
    m_no_space = false;
    return;
}

void SequentialFileWriter::Write(std::string& data)
{
    try {
        m_ofs << data;
    }
    catch (const std::system_error& ex) {
        if (m_ofs.is_open()) {
            m_ofs.close();
        }
        std::remove(m_name.c_str());    // Best effort. We expect it to succeed, but we don't check whether it did
        // RaiseError("writing to", ex);
    }

    data.clear();
    return;
}

// void SequentialFileWriter::RaiseError(const std::string action_attempted, const std::system_error& ex)
// {
//     const int ec = ex.code().value();
//     switch (ec) {
//     case ENOSPC:
//     case EFBIG:
//         m_no_space = true;
//         break;

//     // TODO: Also handle permission errors.
//     default:
//         break;
//     }

//     std::ostringstream sts;
//     sts << "Error " << action_attempted << " the file " << m_name << ": ";
//     raise_from_system_error_code(sts.str(), ec);
// }