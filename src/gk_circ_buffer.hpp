/**
 **  ______  ______  ___   ___  ______  ______  ______  ______
 ** /_____/\/_____/\/___/\/__/\/_____/\/_____/\/_____/\/_____/\
 ** \:::_ \ \::::_\/\::.\ \\ \ \:::_ \ \:::_ \ \::::_\/\:::_ \ \
 **  \:\ \ \ \:\/___/\:: \/_) \ \:\ \ \ \:\ \ \ \:\/___/\:(_) ) )_
 **   \:\ \ \ \::___\/\:. __  ( (\:\ \ \ \:\ \ \ \::___\/\: __ `\ \
 **    \:\/.:| \:\____/\: \ )  \ \\:\_\ \ \:\/.:| \:\____/\ \ `\ \ \
 **     \____/_/\_____\/\__\/\__\/ \_____\/\____/_/\_____\/\_\/ \_\/
 **
 **
 **   If you have downloaded the source code for "Dekoder for Morse" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Dekoder for Morse is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Dekoder is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Dekoder for Morse.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#pragma once

#include "src/defines.hpp"
#include <memory>
#include <thread>
#include <mutex>

namespace GekkoFyre {

template <class T>
class GkCircBuffer {

public:
    explicit GkCircBuffer(const size_t &size) : buf_(std::unique_ptr<T[]>(new T[size])),
                          max_size_(size) { /* empty */ }

    void put(T item);
    T get();
    void reset();
    bool empty() const;
    bool full() const;
    size_t capacity() const;
    size_t size() const;

private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    const size_t max_size_;
    bool full_ = 0;

};

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
void GkCircBuffer<T>::put(T item)
{
    std::lock_guard<std::mutex> lock(mutex_);
    buf_[head_] = item;
    if(full_) {
        tail_ = ((tail_ + 1) % max_size_);
    }

    head_ = ((head_ + 1) % max_size_);
    full_ = head_ == tail_;
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
T GkCircBuffer<T>::GkCircBuffer::get()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(empty()) {
        return T();
    }

    // Read the data and advance the tail (we now have free space!)...
    auto val = buf_[tail_];
    full_ = false;
    tail_ = ((tail_ + 1) % max_size_);

    return val;
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
void GkCircBuffer<T>::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;

    return;
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
bool GkCircBuffer<T>::empty() const
{
    // If head and tail are equal, then we are empty!
    return (!full_ && (head_ == tail_));
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
bool GkCircBuffer<T>::full() const
{
    // If tail is ahead the head by 1, then we are full!
    return full_;

    return false;
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
size_t GkCircBuffer<T>::capacity() const
{
    return max_size_;
}

/**
 * @author Phillip and Rozi <https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/>
 */
template<class T>
size_t GkCircBuffer<T>::size() const
{
    size_t size = max_size_;

    if (!full_) {
        if(head_ >= tail_) {
            size = head_ - tail_;
        } else {
            size = max_size_ + head_ - tail_;
        }
    }

    return size;
}
};
