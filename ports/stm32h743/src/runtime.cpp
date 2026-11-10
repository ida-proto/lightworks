#include <cstddef>
#include <cstdint>

// Freestanding replacements. The firmware links libgcc and nothing from newlib.

extern "C" void* memcpy(void* dest, const void* src, std::size_t count) {
  auto* out = static_cast<unsigned char*>(dest);
  const auto* in = static_cast<const unsigned char*>(src);
  for (std::size_t i = 0; i < count; ++i) {
    out[i] = in[i];
  }
  return dest;
}

extern "C" void* memset(void* dest, int value, std::size_t count) {
  auto* out = static_cast<unsigned char*>(dest);
  const auto byte = static_cast<unsigned char>(value);
  for (std::size_t i = 0; i < count; ++i) {
    out[i] = byte;
  }
  return dest;
}

extern "C" void* memmove(void* dest, const void* src, std::size_t count) {
  auto* out = static_cast<unsigned char*>(dest);
  const auto* in = static_cast<const unsigned char*>(src);
  if (out == in || count == 0) {
    return dest;
  }
  if (out < in) {
    for (std::size_t i = 0; i < count; ++i) {
      out[i] = in[i];
    }
    return dest;
  }
  for (std::size_t i = count; i > 0; --i) {
    out[i - 1] = in[i - 1];
  }
  return dest;
}

extern "C" int memcmp(const void* left, const void* right, std::size_t count) {
  const auto* a = static_cast<const unsigned char*>(left);
  const auto* b = static_cast<const unsigned char*>(right);
  for (std::size_t i = 0; i < count; ++i) {
    if (a[i] != b[i]) {
      return (a[i] < b[i]) ? -1 : 1;
    }
  }
  return 0;
}

extern "C" void __aeabi_memcpy(void* dest, const void* src, std::size_t count) { memcpy(dest, src, count); }
extern "C" void __aeabi_memcpy4(void* dest, const void* src, std::size_t count) { memcpy(dest, src, count); }
extern "C" void __aeabi_memcpy8(void* dest, const void* src, std::size_t count) { memcpy(dest, src, count); }

extern "C" void __aeabi_memmove(void* dest, const void* src, std::size_t count) { memmove(dest, src, count); }
extern "C" void __aeabi_memmove4(void* dest, const void* src, std::size_t count) { memmove(dest, src, count); }
extern "C" void __aeabi_memmove8(void* dest, const void* src, std::size_t count) { memmove(dest, src, count); }

// ARM EABI memset argument order is dest, size, value. C memset is dest, value, size.
extern "C" void __aeabi_memset(void* dest, std::size_t count, int value) { memset(dest, value, count); }
extern "C" void __aeabi_memset4(void* dest, std::size_t count, int value) { memset(dest, value, count); }
extern "C" void __aeabi_memset8(void* dest, std::size_t count, int value) { memset(dest, value, count); }

extern "C" void __aeabi_memclr(void* dest, std::size_t count) { memset(dest, 0, count); }
extern "C" void __aeabi_memclr4(void* dest, std::size_t count) { memset(dest, 0, count); }
extern "C" void __aeabi_memclr8(void* dest, std::size_t count) { memset(dest, 0, count); }

extern "C" void __libc_init_array(void) {
  using InitFn = void (*)();
  extern InitFn __init_array_start[];
  extern InitFn __init_array_end[];
  for (InitFn* fn = __init_array_start; fn != __init_array_end; ++fn) {
    (*fn)();
  }
}
