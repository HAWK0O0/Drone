#pragma once

#include <Arduino.h>

#include "GyroDevice.h"

namespace Espfc {
namespace Device {

class GyroBNO055 : public GyroDevice {
 public:
  DeviceType getType() const override { return GYRO_BNO055; }

  int begin(BusDevice* bus) override
  {
    if(begin(bus, DEFAULT_ADDR)) return 1;
    return begin(bus, ALT_ADDR);
  }

  int begin(BusDevice* bus, uint8_t addr) override
  {
    setBus(bus, addr);
    if(!_bus) return 0;

    if(!waitForChipId()) return 0;

    // Switch to config mode before touching configuration registers.
    if(!_bus->writeByte(_addr, REG_OPR_MODE, MODE_CONFIG)) return 0;
    delay(25);

    // Ensure register PAGE 0.
    (void)_bus->writeByte(_addr, REG_PAGE_ID, 0x00);
    delay(10);

    // Normal power mode.
    (void)_bus->writeByte(_addr, REG_PWR_MODE, PWR_NORMAL);
    delay(10);

    // Units: ACC in mg, GYR in deg/s (default).
    (void)_bus->writeByte(_addr, REG_UNIT_SEL, UNIT_SEL_ACC_MG);
    delay(10);

    // Operation mode: NDOF (with fast magnetometer calibration off).
    (void)_bus->writeByte(_addr, REG_OPR_MODE, MODE_NDOF_FMC_OFF);
    delay(25);

    return 1;
  }

  bool testConnection() override
  {
    if(!_bus) return false;
    uint8_t id = 0;
    const int8_t len = _bus->readByte(_addr, REG_CHIP_ID, &id);
    return len == 1 && id == CHIP_ID;
  }

  int readGyro(VectorInt16& v) override
  {
    uint8_t buffer[6];
    const int8_t len = _bus->readFast(_addr, REG_GYRO_DATA_X_LSB, 6, buffer);
    if(len != 6) return 0;

    v.x = (int16_t)((buffer[1] << 8) | buffer[0]);
    v.y = (int16_t)((buffer[3] << 8) | buffer[2]);
    v.z = (int16_t)((buffer[5] << 8) | buffer[4]);

    return 1;
  }

  int readAccel(VectorInt16& v) override
  {
    uint8_t buffer[6];
    const int8_t len = _bus->readFast(_addr, REG_ACCEL_DATA_X_LSB, 6, buffer);
    if(len != 6) return 0;

    v.x = (int16_t)((buffer[1] << 8) | buffer[0]);
    v.y = (int16_t)((buffer[3] << 8) | buffer[2]);
    v.z = (int16_t)((buffer[5] << 8) | buffer[4]);

    return 1;
  }

  void setDLPFMode(uint8_t mode) override { (void)mode; }
  int getRate() const override { return 100; }
  void setRate(int rate) override { (void)rate; }

 private:
  static constexpr uint8_t DEFAULT_ADDR = 0x28;
  static constexpr uint8_t ALT_ADDR = 0x29;

  static constexpr uint8_t REG_CHIP_ID = 0x00;
  static constexpr uint8_t REG_PAGE_ID = 0x07;
  static constexpr uint8_t REG_ACCEL_DATA_X_LSB = 0x08;
  static constexpr uint8_t REG_GYRO_DATA_X_LSB = 0x14;
  static constexpr uint8_t REG_UNIT_SEL = 0x3B;
  static constexpr uint8_t REG_OPR_MODE = 0x3D;
  static constexpr uint8_t REG_PWR_MODE = 0x3E;

  static constexpr uint8_t CHIP_ID = 0xA0;

  static constexpr uint8_t MODE_CONFIG = 0x00;
  static constexpr uint8_t MODE_NDOF_FMC_OFF = 0x0B;

  static constexpr uint8_t PWR_NORMAL = 0x00;

  // UNIT_SEL bit0 = 1 => ACC in mg (1 LSB = 1mg)
  static constexpr uint8_t UNIT_SEL_ACC_MG = 0x01;

  bool waitForChipId()
  {
    const uint32_t start = millis();
    while(millis() - start < 1000)
    {
      if(testConnection()) return true;
      delay(10);
    }
    return false;
  }
};

} // namespace Device
} // namespace Espfc

