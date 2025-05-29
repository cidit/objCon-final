#pragma once
#include <Arduino.h>

// TODO: document
class IntervalTimer
{
  uint32_t delay_ms, last_ms;

public:
  // TODO: document
  IntervalTimer(uint32_t delay)
      : delay_ms(delay), last_ms(0) {}

  // TODO: document
  bool is_time(uint32_t now)
  {
    return now > this->last_ms + this->delay_ms;
  }

  // TODO: document
  bool is_time_mut(uint32_t now)
  {
    if (this->is_time(now))
    {
      this->last_ms = now;
      return true;
    }
    return false;
  }

  uint32_t get_time_left(uint32_t now)
  {
    return (this->last_ms + this->delay_ms) - now;
  }
};