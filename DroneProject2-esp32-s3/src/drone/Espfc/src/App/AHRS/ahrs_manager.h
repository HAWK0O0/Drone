#pragma once

// AHRS wrapper for the active IMU source (MPU6050 preferred, BNO055 fallback).
// واجهة AHRS لمصدر IMU النشط (يفضّل MPU6050 مع BNO055 كخيار احتياطي).
// 中文: 活动 IMU 的 AHRS 封装（优先 MPU6050，回退到 BNO055）。

#include "INPUT/drone_types.h"

namespace ImuManager {

void begin(float roll_offset_deg, float pitch_offset_deg);
void update();
ImuSnapshot snapshot();
bool isReady();
bool captureLevelOffsets(float* roll_offset_deg, float* pitch_offset_deg);
void setLevelOffsets(float roll_offset_deg, float pitch_offset_deg);
void getLevelOffsets(float* roll_offset_deg, float* pitch_offset_deg);

}  // namespace ImuManager
