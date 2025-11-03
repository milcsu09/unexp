#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <omp.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "tachyon.hpp"

namespace t = tachyon;

////////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr auto PI = 3.1415927;
static constexpr auto PI_2 = PI / 2;
static constexpr auto TAU = PI * 2;

#define RAD(a) ((a) * (PI / 180.0f))
#define DEG(a) ((a) / (PI / 180.0f))

////////////////////////////////////////////////////////////////////////////////////////////////////

struct Gaia_Object
{
  int64_t source_id;
  double ra;
  double dec;
  double parallax;
  double lum_flame;

  Gaia_Object () = default;

  static Gaia_Object
  from (std::string line)
  {
    Gaia_Object object;

    std::stringstream ss (line);
    std::string token;

    std::getline (ss, token, ',');
    object.source_id = std::stoll (token);

    std::getline (ss, token, ',');
    object.ra = std::stod (token);

    std::getline (ss, token, ',');
    object.dec = std::stod (token);

    std::getline (ss, token, ',');
    object.parallax = std::stod (token);

    std::getline (ss, token, ',');
    object.lum_flame = std::stod (token);

    return object;
  }
};

struct Body
{
  t::vector3su position;

  double luminosity;

  Body () = default;

  Body (Gaia_Object object)
  {
    double r_rad = RAD (object.ra);
    double d_rad = RAD (object.dec);
    double distance_pc = object.parallax != 0.0 ? 1000.0 / object.parallax : 0.0;

    position.x = t::spatial_unit::from_pc (distance_pc * cos (d_rad) * cos (r_rad));
    position.y = t::spatial_unit::from_pc (distance_pc * cos (d_rad) * sin (r_rad));
    position.z = t::spatial_unit::from_pc (distance_pc * sin (d_rad));

    luminosity = object.lum_flame;
  }
};

struct Gaia_Source
{
  std::vector<Gaia_Object> objects;

  Gaia_Source () = default;

  bool
  load (std::string path)
  {
    std::ifstream file (path);

    if (!file.is_open ())
      return false;

    std::string line;

    std::getline (file, line); // HEADER

    while (std::getline (file, line))
      objects.push_back (Gaia_Object::from (line));

    if (file.bad ())
      return false;

    if (!file.eof ())
      return false;

    return true;
  }

  std::vector<Body>
  to_bodies () const
  {
    std::vector<Body> bodies;

    bodies.reserve (objects.size ());

    for (const auto &object : objects)
      bodies.emplace_back (object);

    return bodies;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
constexpr uint32_t WW = 800;
constexpr uint32_t WH = 600;
#else
constexpr uint32_t WW = 1920;
constexpr uint32_t WH = 1080;
#endif

constexpr uint32_t FPS = 144;

struct Camera
{
  t::vector3su position;
  double y;
  double p;
  double d;

  double focal_length;

  double f;
  double t;
  double iso;

  double N_photon = 1e6;
};

static sf::RenderWindow window;
static sf::Font font;
static Camera camera;

t::vector3f
project (Camera camera, const t::vector3su &point)
{
  const auto distance = point - camera.position;

  const int64_t tx = distance.x.as_Mm ();
  const int64_t ty = distance.y.as_Mm ();
  const int64_t tz = distance.z.as_Mm ();

  double rx = -ty * cos (camera.y) + tx * sin (camera.y);
  double ry = tx * cos (camera.y) + ty * sin (camera.y);
  double rz = tz * cos (camera.p) - ry * sin (camera.p);

  ry = tz * sin (camera.p) + ry * cos (camera.p);

  if (ry <= 0)
    return tachyon::vector3f::ZERO;

  const auto sx = static_cast<double> (rx) / -ry * camera.d + WW / 2.0;
  const auto sy = static_cast<double> (rz) / -ry * camera.d + WH / 2.0;
  const auto sz = ry;

  return tachyon::vector3f{ sx, sy, sz };
}

void
mark_body (std::string name, t::vector3su position, sf::Color color)
{
  auto p = project (camera, position);

  if (p.z != 0)
    {
      sf::CircleShape circle (5);

      circle.setOrigin ({ 5, 5 });

      circle.setPosition ({ (float)p.x, (float)p.y });

      circle.setOutlineThickness (2);

      circle.setOutlineColor (color);

      circle.setFillColor (sf::Color::Transparent);

      window.draw (circle);

      sf::Text text;

      text.setFont (font);

      text.setString (name);

      text.setCharacterSize (16);

      text.setFillColor (sf::Color{ 128, 128, 128, color.a });

      text.setPosition ({ (float)p.x + 5, (float)p.y + 5 });

      window.draw (text);
    }
}

void
circle_around (sf::VertexArray &vao, t::vector3su origin, t::spatial_unit radius, sf::Color color,
               double inclination_deg = 0.0)
{
  const double inclination_rad = RAD (inclination_deg);

  sf::VertexArray lines (sf::LineStrip);

  for (float phi = 0; phi < 360; phi += 1)
    {
      const double phi_rad = RAD (phi);

      const auto x = radius * cos (phi_rad);
      const auto y = radius * sin (phi_rad) * cos (inclination_rad);
      const auto z = radius * sin (phi_rad) * sin (inclination_rad);

      t::vector3su point;
      point.x = origin.x + x;
      point.y = origin.y + y;
      point.z = origin.z + z;

      auto p = project (camera, point);

      if (p.z != 0)
        {
          sf::Vertex vertex;
          vertex.position = { static_cast<float> (p.x), static_cast<float> (p.y) };
          vertex.color = color;
          vao.append (vertex);
        }
    }

  window.draw (lines);
}

void
center_text_x (sf::Text &text, const sf::Vector2f &position)
{
  const sf::FloatRect bounds = text.getLocalBounds ();

  const int x = bounds.left + bounds.width / 2.0f;

  text.setOrigin (x, 0.0);

  text.setPosition (position);
}

void
center_text_y (sf::Text &text, const sf::Vector2f &position)
{
  const sf::FloatRect bounds = text.getLocalBounds ();

  const int y = bounds.top + bounds.height / 2.0f;

  text.setOrigin (0.0, y);

  text.setPosition (position);
}

void
center_text (sf::Text &text, const sf::Vector2f &position)
{
  const sf::FloatRect bounds = text.getLocalBounds ();

  int x = bounds.left + bounds.width / 2.0f;
  int y = bounds.top + bounds.height / 2.0f;

  text.setOrigin (x, y);

  text.setPosition (position);
}

sf::String
cstr_to_sfstr (const char *s)
{
  return sf::String::fromUtf8 (s, s + std::strlen (s));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void
spatial_unit_as_human (t::spatial_unit unit, char buffer[], size_t size)
{
  int64_t Mm = unit.as_Mm ();
  double AU = unit.as_AU ();
  double ly = unit.as_ly ();
  double kpc = unit.as_kpc ();

  if (abs (kpc) >= 0.1)
    {
      snprintf (buffer, size, "%.2f kpc", kpc);
      return;
    }

  if (abs (ly) >= 0.1)
    {
      snprintf (buffer, size, "%.2f ly", ly);
      return;
    }

  if (abs (AU) >= 0.1)
    {
      snprintf (buffer, size, "%.2f AU", AU);
      return;
    }

  snprintf (buffer, size, "%ld km", Mm * 1000);
}

void
double_to_human (double x, char buffer[], size_t size)
{
  const char *suffix = "";
  double display = x;

  if (fabs (x) >= 1e12)
    {
      display = x / 1e12;
      suffix = "T";
    }

  else if (fabs (x) >= 1e9)
    {
      display = x / 1e9;
      suffix = "G";
    }

  else if (fabs (x) >= 1e6)
    {
      display = x / 1e6;
      suffix = "M";
    }

  else if (fabs (x) >= 1e3)
    {
      display = x / 1e3;
      suffix = "K";
    }

  else if (fabs (x) < 1e-9)
    {
      display = x * 1e12;
      suffix = "p";
    }

  else if (fabs (x) < 1e-6)
    {
      display = x * 1e9;
      suffix = "n";
    }

  else if (fabs (x) < 1e-3)
    {
      display = x * 1e6;
      suffix = "µ";
    }

  else if (fabs (x) < 1)
    {
      display = x * 1e3;
      suffix = "m";
    }

  snprintf (buffer, size, "%.1f%s", display, suffix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static inline double
srgb8_to_linear (uint8_t c)
{
  const double cs = c / 255.0;

  if (cs <= 0.04045)
    return cs / 12.92;
  else
    return std::pow ((cs + 0.055) / 1.055, 2.4);
}

static inline uint8_t
linear_to_srgb8 (double linear)
{
  linear = std::max (0.0, linear);

  double srgb;

  if (linear <= 0.0031308)
    srgb = 12.92 * linear;
  else
    srgb = 1.055 * std::pow (linear, 1.0 / 2.4) - 0.055;

  return static_cast<uint8_t> (std::lround (std::clamp (srgb * 255.0, 0.0, 255.0)));
}

void
applyIntensity_uint8 (uint8_t inR, uint8_t inG, uint8_t inB, double I, uint8_t &outR, uint8_t &outG,
                      uint8_t &outB)
{
  double lr = srgb8_to_linear (inR);
  double lg = srgb8_to_linear (inG);
  double lb = srgb8_to_linear (inB);

  lr *= I;
  lg *= I;
  lb *= I;

  lr = lr / (1.0 + lr);
  lg = lg / (1.0 + lg);
  lb = lb / (1.0 + lb);

  outR = linear_to_srgb8 (lr);
  outG = linear_to_srgb8 (lg);
  outB = linear_to_srgb8 (lb);
}

double
angle_normalize (double a)
{
  return a - (TAU * floor ((a + PI) / TAU));
}


int
main ()
{
  omp_set_num_threads (std::max (omp_get_num_procs () / 2, 1));

  //////////////////////////////////////////////////////////////////////////////////////////////////

  Gaia_Source gaia_source;

  if (!gaia_source.load ("gaia/data.csv"))
    {
      std::cerr << "ERROR: failed to load CSV.\n";
      return 1;
    }

  //////////////////////////////////////////////////////////////////////////////////////////////////

  sf::ContextSettings settings;

  settings.antialiasingLevel = 8;

  window.create (sf::VideoMode{ WW, WH }, "libtachyon", sf::Style::Titlebar, settings);

  window.setPosition ({ 1920 / 2 - WW / 2, 1080 / 2 - WH / 2 });

  window.setFramerateLimit (FPS);

  sf::Mouse::setPosition ({ WW / 2, WH / 2 }, window);

  sf::VertexArray points (sf::Points);
  sf::VertexArray orbits (sf::Points);

  // glPointSize (2.0f); // Gives stars a better look!

  if (!font.loadFromFile ("res/Courier_New.ttf"))
    {
      std::cerr << "ERROR: failed to load font.\n";
      return 1;
    }

  sf::Text text_ft;

  text_ft.setFont (font);
  text_ft.setCharacterSize (24);
  text_ft.setFillColor (sf::Color{ 128, 128, 128 });
  text_ft.setPosition (10.f, 10.f);

  sf::Text text_speed;

  text_speed.setFont (font);
  text_speed.setCharacterSize (24);
  text_speed.setFillColor (sf::Color::White);

  //////////////////////////////////////////////////////////////////////////////////////////////////

  camera.position.x = t::spatial_unit::from_pc (0);
  camera.position.y = t::spatial_unit::from_pc (0);
  camera.position.z = t::spatial_unit::from_pc (0);

  camera.focal_length = 8;

  camera.f = 2.8;
  camera.t = 2.0;
  camera.iso = 1600;

  auto camera_speed = t::spatial_unit::from_Mm (300.0);

  std::vector<Body> bodies = gaia_source.to_bodies ();

  points.resize (bodies.size ());

  bool seeall = false;
  bool orbit_lines = false;

  //////////////////////////////////////////////////////////////////////////////////////////////////

  window.setMouseCursorVisible (false);

  window.setMouseCursorVisible (false);

  const sf::Vector2i center (WW / 2, WH / 2);

  sf::Mouse::setPosition (center, window);
  sf::Clock clock, clock_delta;

  while (window.isOpen ())
    {
      float dt = 1.0 / FPS; // clock_delta.restart ().asSeconds ();

      float start = clock.getElapsedTime ().asSeconds ();

      sf::Event event;

      while (window.pollEvent (event))
        switch (event.type)
          {
          case sf::Event::Closed:
            window.close ();
            break;

          case sf::Event::MouseWheelScrolled:
            if (sf::Mouse::isButtonPressed (sf::Mouse::Right))
              {
                if (event.mouseWheelScroll.delta > 0)
                  camera.focal_length *= 1.1;

                if (event.mouseWheelScroll.delta < 0)
                  camera.focal_length /= 1.1;
              }
            else
              {
                if (event.mouseWheelScroll.delta > 0)
                  camera_speed *= 1.5;

                if (event.mouseWheelScroll.delta < 0)
                  camera_speed /= 1.5;
              }
            break;

          case sf::Event::KeyPressed:
            switch (event.key.code)
              {
              case sf::Keyboard::Tab:
                seeall = !seeall;
                break;

              case sf::Keyboard::O:
                orbit_lines = !orbit_lines;
                break;

              case sf::Keyboard::C:
                camera_speed = t::spatial_unit::from_Mm (300);
                break;

              case sf::Keyboard::V:
                camera_speed = t::spatial_unit::from_ly (1.0);
                break;

              case sf::Keyboard::Add:
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::F))
                  camera.f += 0.1;
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::T))
                  camera.t *= 2.0;
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::I))
                  camera.iso *= 2.0;
                break;

              case sf::Keyboard::Subtract:
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::F))
                  camera.f -= 0.1;
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::T))
                  camera.t /= 2.0;
                if (sf::Keyboard::isKeyPressed (sf::Keyboard::I))
                  camera.iso /= 2.0;
                break;

              default:
                break;
              }
            break;

          case sf::Event::MouseMoved:
            std::cout << event.mouseMove.x << " " << event.mouseMove.y << std::endl;
            break;

          default:
            break;
          }

      if (camera_speed < 2)
        camera_speed = 2;

      double fov = 2 * atan (36 / (2 * camera.focal_length));

      camera.d = WW / (2.0 * tan (fov / 2.0));

      //////////////////////////////////////////////////////////////////////////////////////////////

      auto mouse = sf::Mouse::getPosition (window);

      {
        double dx = WW / 2.0f - mouse.x;
        double dy = WH / 2.0f - mouse.y;

        if (dx != 0 || dy != 0)
          sf::Mouse::setPosition ({ WW / 2, WH / 2 }, window);

        camera.y -= RAD (dx) * .05 * fov;
        camera.p += RAD (dy) * .05 * fov;
      }

      camera.y = angle_normalize (camera.y);
      camera.p = angle_normalize (camera.p);

      auto move_speed = camera_speed * dt;

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::W))
        {
          camera.position.x += std::cos (camera.y) * std::cos (camera.p) * move_speed;
          camera.position.y += std::sin (camera.y) * std::cos (camera.p) * move_speed;
          camera.position.z += std::sin (camera.p) * move_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::S))
        {
          camera.position.x -= std::cos (camera.y) * std::cos (camera.p) * move_speed;
          camera.position.y -= std::sin (camera.y) * std::cos (camera.p) * move_speed;
          camera.position.z -= std::sin (camera.p) * move_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::D))
        {
          camera.position.x += std::cos (camera.y + PI_2) * move_speed;
          camera.position.y += std::sin (camera.y + PI_2) * move_speed;
          camera.position.z += 0;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::A))
        {
          camera.position.x -= std::cos (camera.y + PI_2) * move_speed;
          camera.position.y -= std::sin (camera.y + PI_2) * move_speed;
          camera.position.z -= 0;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::Space))
        {
          camera.position.x += std::cos (camera.y) * -std::sin (camera.p) * move_speed;
          camera.position.y += std::sin (camera.y) * -std::sin (camera.p) * move_speed;
          camera.position.z += std::cos (camera.p) * move_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::LControl))
        {
          camera.position.x -= std::cos (camera.y) * -std::sin (camera.p) * move_speed;
          camera.position.y -= std::sin (camera.y) * -std::sin (camera.p) * move_speed;
          camera.position.z -= std::cos (camera.p) * move_speed;
        }

      //////////////////////////////////////////////////////////////////////////////////////////////

      window.clear ({ 12, 12, 12 });

#pragma omp parallel for schedule(guided)
      for (size_t i = 0; i < bodies.size (); ++i)
        {
          const auto &body = bodies[i];

          const auto dx = (camera.position.x - body.position.x).as_AU ();
          const auto dy = (camera.position.y - body.position.y).as_AU ();
          const auto dz = (camera.position.z - body.position.z).as_AU ();

          const auto L = body.luminosity;
          const auto D = std::sqrt (dx * dx + dy * dy + dz * dz);

          const auto F = 1361.0 * (L / (D * D));

          const auto focal_length = camera.focal_length / 1000;
          const auto f = camera.f;
          const auto t = camera.t;
          const auto iso = camera.iso;
          const auto N_max = camera.N_photon;

          const auto A = PI * std::pow (focal_length / f * 0.5, 2);

          const auto E_photon = 6.626e-34 * 3e8 / 550e-9;
          const auto N_photon = F * A * t / E_photon;

          const auto s = iso / 100;

          auto I = N_photon / N_max * s;

          if (seeall)
            I = 0.4;

          auto p = project (camera, body.position);

          sf::Vertex *point = &points[i];

          if (p.z != 0)
            {
              point->position.x = p.x;
              point->position.y = p.y;

              uint8_t r, g, b;
              applyIntensity_uint8 (255, 115, 60, I, r, g, b);
              point->color.r = r;
              point->color.g = g;
              point->color.b = b;
              point->color.a = std::min (255.0 * I, 255.0);
            }
          else
            {
              point->color.a = 0;
            }
        }

      window.draw (points, sf::BlendMode (sf::BlendAdd));

      const auto dx = camera.position.x.as_AU ();
      const auto dy = camera.position.y.as_AU ();
      const auto dz = camera.position.z.as_AU ();
      const auto d_from_zero = std::sqrt (dx * dx + dy * dy + dz * dz);

      auto au_from_sun = [] (double au) {
        return t::spatial_unit::from_AU (1.0) - t::spatial_unit::from_AU (au);
      };

      uint8_t a_solar;
      uint8_t a_moon;
      float fade_solar = 150.0f;
      float fade_moon = 0.01f;

      if (d_from_zero < fade_moon)
        a_moon = 255;
      else
        a_moon = 255.0 * exp (-(d_from_zero - fade_moon) * 5);

      if (d_from_zero < fade_solar)
        a_solar = 255 - a_moon;
      else
        a_solar = 255.0 * exp (-(d_from_zero - fade_solar) * 0.03);

      mark_body ("Sun", t::vector3su (au_from_sun (0.000), 0, 0),
                 sf::Color{ 255, 204, 51, a_solar });
      mark_body ("Mercury", t::vector3su (au_from_sun (0.387), 0, 0),
                 sf::Color{ 169, 169, 169, a_solar });
      mark_body ("Venus", t::vector3su (au_from_sun (0.723), 0, 0),
                 sf::Color{ 218, 165, 32, a_solar });
      mark_body ("Mars", t::vector3su (au_from_sun (1.524), 0, 0),
                 sf::Color{ 188, 39, 50, a_solar });
      mark_body ("Jupiter", t::vector3su (au_from_sun (5.203), 0, 0),
                 sf::Color{ 216, 179, 130, a_solar });
      mark_body ("Saturn", t::vector3su (au_from_sun (9.537), 0, 0),
                 sf::Color{ 210, 180, 140, a_solar });
      mark_body ("Uranus", t::vector3su (au_from_sun (19.19), 0, 0),
                 sf::Color{ 173, 216, 230, a_solar });
      mark_body ("Neptune", t::vector3su (au_from_sun (30.07), 0, 0),
                 sf::Color{ 63, 84, 186, a_solar });
      mark_body ("Pluto", t::vector3su (au_from_sun (39.50), 0, 0),
                 sf::Color{ 200, 155, 109, a_solar });

      mark_body ("Moon", t::vector3su (au_from_sun (1.00257), 0, 0),
                 sf::Color{ 128, 128, 128, a_moon });
      mark_body ("Earth", t::vector3su (au_from_sun (1.000), 0, 0), sf::Color{ 0, 102, 204 });

      if (orbit_lines)
        {
          t::vector3su sun = { t::spatial_unit::from_AU (1), 0, 0 };

          orbits.clear ();

          circle_around (orbits, sun, t::spatial_unit::from_AU (0.387),
                         sf::Color{ 169, 169, 169, (uint8_t)(1.0 * a_solar) }, 7.00);
          circle_around (orbits, sun, t::spatial_unit::from_AU (0.723),
                         sf::Color{ 218, 165, 32, (uint8_t)(1.0 * a_solar) }, 3.39);
          circle_around (orbits, sun, t::spatial_unit::from_AU (1.000),
                         sf::Color{ 0, 102, 204, (uint8_t)(1.0 * a_solar) }, 0.0);
          circle_around (orbits, sun, t::spatial_unit::from_AU (1.524),
                         sf::Color{ 188, 39, 50, (uint8_t)(1.0 * a_solar) }, 1.85);
          circle_around (orbits, sun, t::spatial_unit::from_AU (5.203),
                         sf::Color{ 216, 179, 130, (uint8_t)(1.0 * a_solar) }, 1.31);
          circle_around (orbits, sun, t::spatial_unit::from_AU (9.537),
                         sf::Color{ 210, 180, 140, (uint8_t)(1.0 * a_solar) }, 2.49);
          circle_around (orbits, sun, t::spatial_unit::from_AU (19.19),
                         sf::Color{ 173, 216, 230, (uint8_t)(1.0 * a_solar) }, 0.77);
          circle_around (orbits, sun, t::spatial_unit::from_AU (30.07),
                         sf::Color{ 63, 84, 186, (uint8_t)(1.0 * a_solar) }, 1.77);
          circle_around (orbits, sun, t::spatial_unit::from_AU (39.5),
                         sf::Color{ 200, 155, 109, (uint8_t)(1.0 * a_solar) }, 17.16);

          circle_around (orbits, t::vector3su::ZERO, t::spatial_unit::from_AU (0.00257),
                         sf::Color{ 128, 128, 128, (uint8_t)(1.0 * a_moon) });

          window.draw (orbits);
        }
      //////////////////////////////////////////////////////////////////////////////////////////////

      {
        sf::CircleShape pointer (1);

        pointer.setPosition ({ WW / 2.0, WH / 2.0 });
        pointer.setFillColor (sf::Color{ 128, 128, 128 });

        window.draw (pointer);
      }

      float end = clock.getElapsedTime ().asSeconds ();

      char buffer_ft[512];

      snprintf (buffer_ft, sizeof buffer_ft,

                "%6.2fms %8.4f\n"
                "focal_length = %8.1fmm ≈ %6.1f°\n"
                "f            = %8.1f\n"
                "t            = %8.1fs\n"
                "ISO          = %8.0f\n"

                "RA           = %.0f°\n"
                "DEC          = %.0f°\n",

                1000.0f * (end - start), dt,
                camera.focal_length, DEG (fov), camera.f, camera.t, camera.iso,

                DEG (camera.y), DEG (camera.p));

      text_ft.setString (cstr_to_sfstr (buffer_ft));

      window.draw (text_ft);

      char buffer_speed[64];
      char buffer_text[512];

      spatial_unit_as_human (camera_speed, buffer_speed, sizeof buffer_speed);

      char buffer_c[64];
      double_to_human (camera_speed.as_Mm () / 300.0, buffer_c, sizeof buffer_c);

      snprintf (buffer_text, sizeof buffer_text, "%s / second\n%sc", buffer_speed, buffer_c);

      text_speed.setString (cstr_to_sfstr (buffer_text));

      center_text_x (text_speed, sf::Vector2f{ WW / 2.0, 10 });

      window.draw (text_speed);

      //////////////////////////////////////////////////////////////////////////////////////////////

      window.display ();
    }
}

