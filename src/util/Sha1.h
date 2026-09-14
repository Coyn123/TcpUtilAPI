#pragma once
#include <array>
#include <cstddef>

std::array<unsigned char, 20> sha1(const unsigned char* data, size_t len);
