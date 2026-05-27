#pragma once

// PID Controller Class
// فئة متحكم PID
// 中文: PID 控制器类
// Implements Proportional-Integral-Derivative control algorithm
// تنفيذ خوارزمية التحكم التناسبي-التكاملي-المشتق
// 中文: 实现比例-积分-微分控制算法

#include "Control/controller_math.h"
#include "INPUT/drone_config.h"
#include "INPUT/drone_types.h"

class PidController {
 public:
  void Configure(const PidTuning& tuning) { tuning_ = tuning; }

  void Reset() {
    integral_ = 0.0f;
    last_output_ = 0.0f;
    filtered_rate_dps_ = 0.0f;
  }

  float Update(float error, float measured_rate_dps, float dt_seconds) {
    if (dt_seconds <= 0.0f) return 0.0f;

    // Anti-windup: clamp integral based on error sign to prevent overshoot
    // مكافحة التراكم: تحديد التكامل بناءً على علامة الخطأ لمنع الإفراط
    // 中文: 防积分饱和：根据错误符号限制积分，防止超调
    const float ki_max = tuning_.integral_limit / (fabsf(tuning_.ki) > 0.001f ? tuning_.ki : 0.001f);
    integral_ += error * dt_seconds;
    integral_ = ControllerMath::Clamp(integral_, -ki_max, ki_max);

    // Derivative filter: low-pass filter on rate to reduce sensor noise
    // فلتر المشتقة: فلتر تمرير منخفض على معدل تقليل ضجيج المستشعر
    // 中文: 导数滤波器：对速率进行低通滤波，减少传感器噪声
    const float derivative_filter_alpha = ControllerMath::Clamp(
      DroneConfig::kPidDerivativeFilterAlpha, 0.01f, 1.0f);
    filtered_rate_dps_ = filtered_rate_dps_ * (1.0f - derivative_filter_alpha) +
               measured_rate_dps * derivative_filter_alpha;

    // P term: proportional to angles
    // حد التناسب: متناسب مع الزوايا
    // 中文: P 项：与角度成正比
    const float p_term = tuning_.kp * error;

    // I term: integral of error
    // حد التكامل: تكامل الخطأ
    // 中文: I 项：误差的积分
    const float i_term = tuning_.ki * integral_;

    // D term: negative feedback from filtered rate (derivative of angle feedback)
    // حد المشتقة: تغذية عكسية سالبة من المعدل المصفى
    // 中文: D 项：来自滤波速率的负反馈（角度反馈的导数）
    const float d_term = -tuning_.kd * filtered_rate_dps_;

    float output = p_term + i_term + d_term;

    // Rate limiter: smooth output changes to prevent jerky movements
    // محدد المعدل: تجعيد سلس لتغييرات الإخراج لمنع الحركات المتشنجة
    // 中文: 变化率限制器：平滑输出变化，防止急剧运动
    const float max_change =
      DroneConfig::kPidRateLimiterMaxChangePerSec * dt_seconds;
    const float delta = output - last_output_;
    if (fabsf(delta) > max_change) {
      output = last_output_ + (delta > 0.0f ? max_change : -max_change);
    }

    // Final saturation
    // التشبع النهائي
    // 中文: 最终饱和
    last_output_ = ControllerMath::Clamp(output, -tuning_.output_limit, tuning_.output_limit);
    return last_output_;
  }

  float last_output() const { return last_output_; }
  float integral() const { return integral_; }

 private:
  PidTuning tuning_{};
  float integral_ = 0.0f;
  float last_output_ = 0.0f;
  float filtered_rate_dps_ = 0.0f;           // Low-pass filtered derivative
};
