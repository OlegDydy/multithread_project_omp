#pragma once

#include <string>

struct processor_info{
  std::wstring name;
  uint32_t freqency;
  uint8_t cores;
  uint8_t threads;
  uint32_t L1;
  uint32_t L2;
  uint32_t L3;
};

processor_info get_processor_info();