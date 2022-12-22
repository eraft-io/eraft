// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <condition_variable>

template<class T>

class channel {

public:

    channel() {};

    virtual ~channel() {};

protected:

    channel& operator=(const channel& other ) = delete;

    channel(const channel& other) = delete;

public:

    T recevive_blocking() 
    {
        std::unique_lock<std::mutex> the_lock(m_mutex);

        m_cv.wait(the_lock, [this]{
            return m_has_value;
        });

        return m_val;
    }    

    void send(T&& val)
    {
        {
            std::unique_lock<std::mutex> the_lock(m_mutex);
            m_val = val;
            m_has_value = true;
        }
        m_cv.notify_all();
    };

protected:

    T m_val;

    bool m_has_value {false};

    std::mutex m_mutex;

    std::condition_variable m_cv;
};
