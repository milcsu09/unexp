#ifndef TACHYON_HPP
#define TACHYON_HPP

#include <cmath>
#include <cstdint>
#include <utility>

namespace tachyon
{
class spatial_unit
{
private:
  int64_t m_value{ 0 };

public:
  static constexpr int64_t AU = 1.49597e5;
  static constexpr int64_t LY = 9.460730472e9;
  static constexpr int64_t PC = 3.0856775814e10;
  static constexpr int64_t KPC = PC * 1e3;

  spatial_unit () = default;

  spatial_unit (int64_t mm);

  static spatial_unit from_Mm (int64_t mm);
  static spatial_unit from_AU (double au);
  static spatial_unit from_ly (double ly);
  static spatial_unit from_pc (double pc);
  static spatial_unit from_kpc (double kpc);

  int64_t as_Mm () const;
  double as_AU () const;
  double as_ly () const;
  double as_pc () const;
  double as_kpc () const;

  bool operator== (const spatial_unit &other) const;
  bool operator!= (const spatial_unit &other) const;
  bool operator< (const spatial_unit &other) const;
  bool operator> (const spatial_unit &other) const;
  bool operator<= (const spatial_unit &other) const;
  bool operator>= (const spatial_unit &other) const;

  spatial_unit operator+ (const spatial_unit &other) const;
  spatial_unit operator- (const spatial_unit &other) const;
  spatial_unit operator- () const;
  spatial_unit operator* (double scalar) const;
  spatial_unit operator/ (double scalar) const;

  spatial_unit &operator+= (const spatial_unit &other);
  spatial_unit &operator-= (const spatial_unit &other);
  spatial_unit &operator*= (double scalar);
  spatial_unit &operator/= (double scalar);

  friend spatial_unit operator* (double scalar, const spatial_unit &unit);
};

template <typename T> struct vector3
{
  static const vector3<T> ZERO;

  T x{ 0 }, y{ 0 }, z{ 0 };

  vector3 () = default;

  vector3 (T v);
  vector3 (T _x, T _y, T _z);
  vector3 (const vector3<T> &other);
  vector3 (vector3<T> &&other) noexcept;

  vector3<T> &operator= (const vector3<T> &other);
  vector3<T> &operator= (vector3<T> &&other) noexcept;

  T distance (const vector3<T> &other) const;

  vector3<T> operator+ (const vector3<T> &other) const;
  vector3<T> operator- (const vector3<T> &other) const;
  vector3<T> operator* (double scalar) const;
  vector3<T> operator/ (double scalar) const;

  vector3<T> &operator+= (const vector3<T> &other);
  vector3<T> &operator-= (const vector3<T> &other);
  vector3<T> &operator*= (double scalar);
  vector3<T> &operator/= (double scalar);

  template <typename U>
  friend vector3<U> operator* (double scalar, const vector3<U> &v);
};

typedef vector3<int64_t> vector3i;
typedef vector3<double> vector3f;
typedef vector3<spatial_unit> vector3su;

template <typename T> const vector3<T> vector3<T>::ZERO = vector3<T> (0, 0, 0);

template <typename T> vector3<T>::vector3 (T v) : x (v), y (v), z (v) {}

template <typename T>
vector3<T>::vector3 (T _x, T _y, T _z) : x (_x), y (_y), z (_z)
{
}

template <typename T>
vector3<T>::vector3 (const vector3<T> &other)
    : x (other.x), y (other.y), z (other.z)
{
}

template <typename T>
vector3<T>::vector3 (vector3<T> &&other) noexcept : x (std::move (other.x)),
                                                    y (std::move (other.y)),
                                                    z (std::move (other.z))
{
}

template <typename T>
vector3<T> &
vector3<T>::operator= (const vector3<T> &other)
{
  if (this != &other)
    x = other.x, y = other.y, z = other.z;
  return *this;
}

template <typename T>
vector3<T> &
vector3<T>::operator= (vector3<T> &&other) noexcept
{
  if (this != &other)
    x = std::move (other.x), y = std::move (other.y), z = std::move (other.z);
  return *this;
}

template <typename T>
T
vector3<T>::distance (const vector3<T> &other) const
{
  const auto dx = x - other.x;
  const auto dy = y - other.y;
  const auto dz = z - other.z;
  return std::sqrt (dx * dx + dy * dy + dz * dz);
}

template <typename T>
vector3<T>
vector3<T>::operator+ (const vector3<T> &other) const
{
  return vector3 (x + other.x, y + other.y, z + other.z);
}

template <typename T>
vector3<T>
vector3<T>::operator- (const vector3<T> &other) const
{
  return vector3 (x - other.x, y - other.y, z - other.z);
}

template <typename T>
vector3<T>
vector3<T>::operator* (double scalar) const
{
  return vector3<T> (x * scalar, y * scalar, z * scalar);
}

template <typename T>
vector3<T>
vector3<T>::operator/ (double scalar) const
{
  return vector3<T> (x / scalar, y / scalar, z / scalar);
}

template <typename T>
vector3<T> &
vector3<T>::operator+= (const vector3<T> &other)
{
  x += other.x;

  y += other.y;
  z += other.z;
  return *this;
}

template <typename T>
vector3<T> &
vector3<T>::operator-= (const vector3<T> &other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

template <typename T>
vector3<T> &
vector3<T>::operator*= (double scalar)
{
  x *= scalar;
  y *= scalar;
  z *= scalar;
  return *this;
}

template <typename T>
vector3<T> &
vector3<T>::operator/= (double scalar)
{
  x /= scalar;
  y /= scalar;
  z /= scalar;
  return *this;
}

template <typename T>
vector3<T>
operator* (double scalar, const vector3<T> &v)
{
  return v * scalar;
}
}; // namespace tachyon

#endif // TACHYON_HPP

