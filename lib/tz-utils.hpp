#pragma once
#include <cstdint>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i32 = int32_t;
using i64 = int64_t;

#define LOG_DEBUG(fmt, ...)                                                    \
  std::cout << std::format(fmt, __VA_ARGS__) << std::endl

class NonCopyable {
public:
  NonCopyable() = default;
  NonCopyable(const NonCopyable &) = delete;
  NonCopyable &operator=(const NonCopyable &) = delete;
};

class NonMoveable {
public:
  NonMoveable() = default;
  NonMoveable(NonMoveable &&) = delete;
  NonMoveable &operator=(NonMoveable &&) = delete;
};

class MappedFile : NonCopyable {
public:
  MappedFile(std::string_view _filename) {
    // for null termination
    std::string filename = std::string(_filename);
    fd = open(filename.data(), O_RDONLY);
    if (fd == -1) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      close(fd);
      throw std::runtime_error("Failed to get file size: " + filename);
    }

    length = sb.st_size;

    addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
      close(fd);
      throw std::runtime_error("Failed to map file: " + filename);
    }
  }

  MappedFile(MappedFile &&other) noexcept {
    fd = other.fd;
    addr = other.addr;
    length = other.length;
    other.fd = -1;
    other.addr = nullptr;
    other.length = 0;
  }

  MappedFile &operator=(MappedFile &&other) noexcept {
    if (this != &other) {
      fd = other.fd;
      addr = other.addr;
      length = other.length;
      other.fd = -1;
      other.addr = nullptr;
      other.length = 0;
    }
    return *this;
  }

  ~MappedFile() {
    munmap(addr, length);
    close(fd);
  }

  void dump() const {
    for (std::size_t i = 0; i < length; i++) {
      printf("%02x ", data()[i]);
    }
    printf("\n");
  }

  const uint8_t *data() const { return static_cast<const uint8_t *>(addr); }

  std::size_t size() const { return length; }

  std::span<const u8> asSpan() const { return {data(), length}; }

private:
  int fd;
  void *addr;
  std::size_t length;
};
