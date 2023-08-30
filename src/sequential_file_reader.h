/**
 * @file sequential_file_reader.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <functional>

// SequentialFileReader: Read a file using using mmap(). Attempt to overlap reads of the file and writes by the user's code
// by reading the next segment 

class SequentialFileReader {
public:
    SequentialFileReader(SequentialFileReader&&);
    SequentialFileReader& operator=(SequentialFileReader&&);
    ~SequentialFileReader();

    // TODO: Provide some way to log non-critical errors which don't prevent the actual reading
    // of data, but could hurt performance

    // Read the file, calling OnChunkAvailable() whenever data are available. It blocks until the reading
    // is complete.
    void Read(size_t max_chunk_size);

    std::string GetFilePath() const
    {
        return m_file_path;
    }

protected:
    // Constructor. Attempts to open the file, and throws std::system_error if it fails to do so.
    SequentialFileReader(const std::string& file_name);

    // TODO: Also provide a constructor that doesn't open the file, and a separate Open method.

    // OnChunkAvailable: The user needs to override this function to get called when data become available.
    virtual void OnChunkAvailable(const void* data, size_t size) = 0;

private:
    std::string m_file_path;
    std::unique_ptr< const std::uint8_t, std::function<void(const std::uint8_t*)> > m_data;
    size_t m_size;
};