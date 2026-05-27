#include "RC/rc_receiver.h"

#include <HardwareSerial.h>
#include <driver/uart.h>
#include <esp_timer.h>

#include "INPUT/drone_config.h"

namespace {

constexpr size_t kRcChannelCount = 8;
constexpr size_t kSbusFrameSize = 25;
constexpr uint8_t kSbusStartByte = 0x0F;
constexpr uint16_t kSbusRawMin = 172;
constexpr uint16_t kSbusRawMax = 1811;
constexpr uint8_t kSbusFlagFrameLost = 1u << 2;
constexpr uint8_t kSbusFlagFailsafe = 1u << 3;

HardwareSerial g_sbus_serial(DroneConfig::kRcSbusUart);
uint16_t g_channel_us[kRcChannelCount];
uint32_t g_last_update_us[kRcChannelCount];
uint8_t g_frame[kSbusFrameSize];
size_t g_frame_index = 0;

uint16_t DecodeSbusChannel(const uint8_t* frame, uint8_t channel_index) {
  const uint32_t bit_index = static_cast<uint32_t>(channel_index) * 11U;
  const uint32_t byte_index = 1U + (bit_index / 8U);
  const uint8_t shift = bit_index % 8U;
  const uint32_t packed = static_cast<uint32_t>(frame[byte_index]) |
                          (static_cast<uint32_t>(frame[byte_index + 1]) << 8U) |
                          (static_cast<uint32_t>(frame[byte_index + 2]) << 16U);
  return static_cast<uint16_t>((packed >> shift) & 0x07FFU);
}

uint16_t SbusRawToUs(uint16_t raw_value) {
  const long scaled = map(static_cast<long>(raw_value), static_cast<long>(kSbusRawMin),
                          static_cast<long>(kSbusRawMax),
                          static_cast<long>(DroneConfig::kRcMinUs),
                          static_cast<long>(DroneConfig::kRcMaxUs));
  return static_cast<uint16_t>(constrain(scaled, static_cast<long>(DroneConfig::kRcMinUs),
                                         static_cast<long>(DroneConfig::kRcMaxUs)));
}

void ApplySbusFrame(const uint8_t* frame) {
  const uint8_t flags = frame[23];
  if ((flags & (kSbusFlagFrameLost | kSbusFlagFailsafe)) != 0) {
    return;
  }

  const uint32_t now_us = static_cast<uint32_t>(esp_timer_get_time());
  for (uint8_t i = 0; i < kRcChannelCount; ++i) {
    g_channel_us[i] = SbusRawToUs(DecodeSbusChannel(frame, i));
    g_last_update_us[i] = now_us;
  }
}

void PollSbus() {
  while (g_sbus_serial.available() > 0) {
    const int read_value = g_sbus_serial.read();
    if (read_value < 0) {
      break;
    }

    const uint8_t byte = static_cast<uint8_t>(read_value);
    if (g_frame_index == 0) {
      if (byte != kSbusStartByte) {
        continue;
      }
      g_frame[g_frame_index++] = byte;
      continue;
    }

    g_frame[g_frame_index++] = byte;
    if (g_frame_index < kSbusFrameSize) {
      continue;
    }

    ApplySbusFrame(g_frame);
    g_frame_index = 0;
  }
}

}  // namespace

// Initialize SBUS receiver UART
// تهيئة UART لمستقبل SBUS
// 中文: 初始化 SBUS 接收机 UART
void RcInput::begin() {
  const uint32_t now_us = static_cast<uint32_t>(esp_timer_get_time());

  for (uint8_t i = 0; i < kRcChannelCount; ++i) {
    g_channel_us[i] = (i == DroneConfig::kRcThrottleIndex) ? DroneConfig::kRcMinUs
                                                           : DroneConfig::kRcMidUs;
    g_last_update_us[i] = now_us;
  }

  g_frame_index = 0;
  g_sbus_serial.begin(DroneConfig::kRcSbusBaud, SERIAL_8E2, DroneConfig::kRcSbusRxPin, -1);
  uart_set_line_inverse(static_cast<uart_port_t>(DroneConfig::kRcSbusUart),
                        DroneConfig::kRcSbusInvert ? UART_SIGNAL_RXD_INV : 0);
}

RcSnapshot RcInput::snapshot() {
  PollSbus();

  RcSnapshot snapshot{};
  const uint32_t now_us = static_cast<uint32_t>(esp_timer_get_time());
  constexpr uint8_t kRequiredChannels[] = {
      DroneConfig::kRcRollIndex,
      DroneConfig::kRcPitchIndex,
      DroneConfig::kRcThrottleIndex,
      DroneConfig::kRcYawIndex,
      DroneConfig::kRcArmIndex,
  };

  for (uint8_t i = 0; i < kRcChannelCount; ++i) {
    snapshot.channels[i] = g_channel_us[i];
    snapshot.fresh[i] = (now_us - g_last_update_us[i]) <= DroneConfig::kRcFailsafeUs;
  }

  snapshot.frame_fresh = true;
  for (uint8_t i = 0; i < sizeof(kRequiredChannels); ++i) {
    snapshot.frame_fresh = snapshot.frame_fresh && snapshot.fresh[kRequiredChannels[i]];
  }
  return snapshot;
}

uint16_t RcInput::readChannel(uint8_t index, uint16_t fallback) {
  PollSbus();
  if (index >= kRcChannelCount) return fallback;
  const uint16_t value = g_channel_us[index];
  if (value < DroneConfig::kRcPulseMinUs || value > DroneConfig::kRcPulseMaxUs) {
    return fallback;
  }
  return value;
}

bool RcInput::channelFresh(uint8_t index) {
  PollSbus();
  if (index >= kRcChannelCount) return false;
  const uint32_t now_us = static_cast<uint32_t>(esp_timer_get_time());
  const uint32_t last_update = g_last_update_us[index];
  return (now_us - last_update) <= DroneConfig::kRcFailsafeUs;
}

bool RcInput::frameFresh() {
  PollSbus();

  constexpr uint8_t kRequiredChannels[] = {
      DroneConfig::kRcRollIndex,
      DroneConfig::kRcPitchIndex,
      DroneConfig::kRcThrottleIndex,
      DroneConfig::kRcYawIndex,
      DroneConfig::kRcArmIndex,
  };
  for (uint8_t i = 0; i < sizeof(kRequiredChannels); ++i) {
    if (!channelFresh(kRequiredChannels[i])) return false;
  }
  return true;
}
