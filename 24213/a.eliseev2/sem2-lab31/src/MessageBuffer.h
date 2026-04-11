#pragma once

#include "Scheduler.h"
#include <vector>

namespace proxy {

class MessageBuffer {
  public:
    class Reader {
      public:
        Reader(MessageBuffer &buffer, size_t &position);

        Reader(const Reader &) = delete;
        Reader &operator=(const Reader &) = delete;

        const char *data();
        size_t length();
        bool isEnd();
        void advance(size_t size);

        void await(Waker waker);

      private:
        size_t *position;
        MessageBuffer *buffer;
    };

    class Writer {
      public:
        Writer(MessageBuffer &buffer);
        ~Writer();
        
        Writer(const Writer &) = delete;
        Writer &operator=(const Writer &) = delete;

        char *reserve(size_t size);
        void commit(size_t size);
        void end();

      private:
        size_t reservedSize;
        MessageBuffer *buffer;
    };

    MessageBuffer();

    Reader read(size_t &position);
    Writer write();

  private:
    void await(Waker waker);
    void scheduleAwaiters();

    std::vector<char> data;
    bool isEnd;
    std::vector<Waker> awaiters;
};

} // namespace proxy
