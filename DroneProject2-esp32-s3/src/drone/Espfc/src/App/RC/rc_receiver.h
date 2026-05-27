#pragma once

// SBUS receiver capture for CH1..CH8.
// قراءة SBUS للريسيفر لقنوات CH1..CH8.
// 中文: 用于 CH1..CH8 的 SBUS 接收机采样。

#include "INPUT/drone_types.h"

namespace RcInput {

void begin();
RcSnapshot snapshot();
uint16_t readChannel(uint8_t index, uint16_t fallback);
bool channelFresh(uint8_t index);
bool frameFresh();

}  // namespace RcInput
