#include "tachyon.hpp"

namespace tachyon
{
spatial_unit::spatial_unit (int64_t mm) : m_value (mm) {}

spatial_unit
spatial_unit::from_Mm (int64_t mm)
{
  return spatial_unit (mm);
}

spatial_unit
spatial_unit::from_AU (double au)
{
  return spatial_unit (static_cast<int64_t> (au * AU));
}

spatial_unit
spatial_unit::from_ly (double ly)
{
  return spatial_unit (static_cast<int64_t> (ly * LY));
}

spatial_unit
spatial_unit::from_pc (double pc)
{
  return spatial_unit (static_cast<int64_t> (pc * PC));
}

spatial_unit
spatial_unit::from_kpc (double kpc)
{
  return spatial_unit (static_cast<int64_t> (kpc * KPC));
}

int64_t
spatial_unit::as_Mm () const
{
  return m_value;
}

double
spatial_unit::as_AU () const
{
  return static_cast<double> (m_value) / AU;
}

double
spatial_unit::as_ly () const
{
  return static_cast<double> (m_value) / LY;
}

double
spatial_unit::as_pc () const
{
  return static_cast<double> (m_value) / PC;
}

double
spatial_unit::as_kpc () const
{
  return static_cast<double> (m_value) / KPC;
}

bool
spatial_unit::operator== (const spatial_unit &other) const
{
  return m_value == other.m_value;
}

bool
spatial_unit::operator!= (const spatial_unit &other) const
{
  return !(*this == other);
}

bool
spatial_unit::operator< (const spatial_unit &other) const
{
  return m_value < other.m_value;
}

bool
spatial_unit::operator> (const spatial_unit &other) const
{
  return m_value > other.m_value;
}

bool
spatial_unit::operator<= (const spatial_unit &other) const
{
  return m_value <= other.m_value;
}

bool
spatial_unit::operator>= (const spatial_unit &other) const
{
  return m_value >= other.m_value;
}

spatial_unit
spatial_unit::operator+ (const spatial_unit &other) const
{
  return spatial_unit (m_value + other.m_value);
}

spatial_unit
spatial_unit::operator- (const spatial_unit &other) const
{
  return spatial_unit (m_value - other.m_value);
}

spatial_unit
spatial_unit::operator- () const
{
  return -m_value;
}

spatial_unit
spatial_unit::operator* (double scalar) const
{
  return spatial_unit (static_cast<int64_t> (m_value * scalar));
}

spatial_unit
spatial_unit::operator/ (double scalar) const
{
  return spatial_unit (static_cast<int64_t> (m_value / scalar));
}

spatial_unit &
spatial_unit::operator+= (const spatial_unit &other)
{
  m_value += other.m_value;
  return *this;
}

spatial_unit &
spatial_unit::operator-= (const spatial_unit &other)
{
  m_value -= other.m_value;
  return *this;
}

spatial_unit &
spatial_unit::operator*= (double scalar)
{
  m_value = static_cast<int64_t> (m_value * scalar);
  return *this;
}

spatial_unit &
spatial_unit::operator/= (double scalar)
{
  m_value = static_cast<int64_t> (m_value / scalar);
  return *this;
}

spatial_unit
operator* (double scalar, const spatial_unit &unit)
{
  return unit * scalar;
}
} // namespace tachyon

