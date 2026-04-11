#include "MessageBuffer.h"

namespace proxy {

MessageBuffer::MessageBuffer() : data(), isEnd(false), awaiters() {
}

MessageBuffer::Reader MessageBuffer::read(size_t &position) {
    return Reader(*this, position);
}

MessageBuffer::Writer MessageBuffer::write() {
    return Writer(*this);
}

void MessageBuffer::await(Waker waker) {
    awaiters.push_back(waker);
}

void MessageBuffer::scheduleAwaiters() {
    for (auto awaiter : awaiters) {
        awaiter.schedule();
    }
    awaiters.clear();
}

MessageBuffer::Reader::Reader(MessageBuffer &buffer,
                                          size_t &position)
    : buffer(&buffer), position(&position) {
}

const char *MessageBuffer::Reader::data() {
    return buffer->data.data() + *position;
}

size_t MessageBuffer::Reader::length() {
    return buffer->data.size() - *position;
}

bool MessageBuffer::Reader::isEnd() {
    return (buffer->data.size() - *position) == 0 && buffer->isEnd;
}

void MessageBuffer::Reader::advance(size_t size) {
    *position = std::min(buffer->data.size(), *position + size);
}

void MessageBuffer::Reader::await(Waker waker) {
    buffer->await(waker);
}

MessageBuffer::Writer::Writer(MessageBuffer &buffer)
    : buffer(&buffer), reservedSize(0) {
}

MessageBuffer::Writer::~Writer() {
    buffer->scheduleAwaiters();
}

char *MessageBuffer::Writer::reserve(size_t size) {
    size_t oldSize = buffer->data.size() - reservedSize;
    buffer->data.resize(oldSize + size);
    reservedSize = size;
    return buffer->data.data() + oldSize;
}

void MessageBuffer::Writer::commit(size_t size) {
    buffer->data.resize(buffer->data.size() - reservedSize + size);
    reservedSize = 0;
}

void MessageBuffer::Writer::end() {
    buffer->isEnd = true;
}

} // namespace proxy
