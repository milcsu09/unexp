#include <SFML/Graphics.hpp>
#include <omp.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "tachyon.hpp"

namespace t = tachyon;


////////////////////////////////////////////////////////////////////////////////////////////////////


static constexpr auto PI   = 3.1415927;
static constexpr auto PI_2 = PI / 2;

#define RAD(a) ((a) * (PI / 180.0f))
#define DEG(a) ((a) / (PI / 180.0f))


////////////////////////////////////////////////////////////////////////////////////////////////////


struct Gaia_Object
{
  double r;
  double d;
  double distance;

  Gaia_Object () = default;

  static Gaia_Object
  from (std::string line)
  {
    Gaia_Object object;

    std::stringstream ss (line);
    std::string token;

    std::getline (ss, token, ','); // SOURCE_ID

    std::getline (ss, token, ',');
    object.r = std::stod (token);

    std::getline (ss, token, ',');
    object.d = std::stod (token);

    std::getline (ss, token, ',');
    object.distance = std::stod (token);

    return object;
  }
};


struct Body
{
  t::vector3su position;

  Body () = default;

  Body (Gaia_Object object)
  {
    double r_rad = RAD (object.r);
    double d_rad = RAD (object.d);
    double distance_pc = object.distance != 0.0 ? 1000.0 / object.distance : 0.0;

    position.x = t::spatial_unit::from_pc (distance_pc * cos (d_rad) * cos (r_rad));
    position.y = t::spatial_unit::from_pc (distance_pc * cos (d_rad) * sin (r_rad));
    position.z = t::spatial_unit::from_pc (distance_pc * sin (d_rad));
  }
};


struct Gaia_Source
{
  std::vector<Gaia_Object> objects;

  Gaia_Source () = default;

  bool
  read (std::string path)
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

    if (!file.eof())
      return false;

    return true;
  }

  std::vector<Body>
  to_bodies () const
  {
    std::vector<Body> bodies;

    bodies.reserve (objects.size());

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


struct Camera
{
  t::vector3su position;
  double y;
  double p;
  double d;
};


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


////////////////////////////////////////////////////////////////////////////////////////////////////


int
main ()
{
  omp_set_num_threads (std::max (omp_get_num_procs () / 3, 1));

  //////////////////////////////////////////////////////////////////////////////////////////////////

  Gaia_Source gaia_source;

  if (!gaia_source.read ("gaia/data.csv"))
    {
      std::cerr << "ERROR: failed to read CSV file.\n";
      return 1;
    }

  //////////////////////////////////////////////////////////////////////////////////////////////////

  sf::ContextSettings settings;

  settings.antialiasingLevel = 8;

  sf::RenderWindow window{ sf::VideoMode{ WW, WH }, "libtachyon",
                           sf::Style::Titlebar | sf::Style::Close, settings };

  window.setFramerateLimit (144);

  {
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode ();

    auto x = (desktop.width - WW) / 2.0;
    auto y = (desktop.height - WH) / 2.0;

    window.setPosition (sf::Vector2i (x, y));
  }

  sf::VertexArray points(sf::Points);

  //////////////////////////////////////////////////////////////////////////////////////////////////

  Camera camera;

  camera.position.x = t::spatial_unit::from_pc (0);
  camera.position.y = t::spatial_unit::from_pc (0);
  camera.position.z = t::spatial_unit::from_pc (0);

  auto camera_fov = 130.0;
  auto camera_speed = tachyon::spatial_unit::from_AU (1.0);

  std::vector<Body> bodies = gaia_source.to_bodies ();

  points.resize (bodies.size ());

  bool seeall = false;

  //////////////////////////////////////////////////////////////////////////////////////////////////

  sf::Clock clock;

  while (window.isOpen ())
    {
      // float dt = clock.restart ().asSeconds ();

      sf::Event event;

      while (window.pollEvent (event))
        switch (event.type)
          {
          case sf::Event::Closed:
            window.close ();
            break;

          case sf::Event::MouseWheelScrolled:
            if (event.mouseWheelScroll.delta > 0)
              camera_speed *= 1.5;

            if (event.mouseWheelScroll.delta < 0)
              camera_speed /= 1.5;
            break;

          case sf::Event::KeyPressed:
            switch (event.key.code)
              {
              case sf::Keyboard::Tab:
                seeall = !seeall;
                break;

              default:
                break;
              }
            break;

          default:
            break;
          }

      camera.d = WW / (2.0 * tan (RAD (camera_fov) / 2.0));

      //////////////////////////////////////////////////////////////////////////////////////////////

      auto mouse = sf::Mouse::getPosition (window);

      {
        auto dx = WW / 2.0f - mouse.x;
        auto dy = WH / 2.0f - mouse.y;

        if (dx != 0 || dy != 0)
          sf::Mouse::setPosition ({ WW / 2, WH / 2 }, window);

        camera.y -= RAD (dx) * .05 * RAD (camera_fov);
        camera.p += RAD (dy) * .05 * RAD (camera_fov);
      }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::W))
        {
          camera.position.x += std::cos (camera.y) * std::cos (camera.p) * camera_speed;
          camera.position.y += std::sin (camera.y) * std::cos (camera.p) * camera_speed;
          camera.position.z += std::sin (camera.p) * camera_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::S))
        {
          camera.position.x -= std::cos (camera.y) * std::cos (camera.p) * camera_speed;
          camera.position.y -= std::sin (camera.y) * std::cos (camera.p) * camera_speed;
          camera.position.z -= std::sin (camera.p) * camera_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::D))
        {
          camera.position.x += std::cos (camera.y + PI_2) * camera_speed;
          camera.position.y += std::sin (camera.y + PI_2) * camera_speed;
          camera.position.z += 0;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::A))
        {
          camera.position.x -= std::cos (camera.y + PI_2) * camera_speed;
          camera.position.y -= std::sin (camera.y + PI_2) * camera_speed;
          camera.position.z -= 0;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::Space))
        {
          camera.position.x += std::cos (camera.y) * -std::sin (camera.p) * camera_speed;
          camera.position.y += std::sin (camera.y) * -std::sin (camera.p) * camera_speed;
          camera.position.z += std::cos (camera.p) * camera_speed;
        }

      if (sf::Keyboard::isKeyPressed (sf::Keyboard::LControl))
        {
          camera.position.x -= std::cos (camera.y) * -std::sin (camera.p) * camera_speed;
          camera.position.y -= std::sin (camera.y) * -std::sin (camera.p) * camera_speed;
          camera.position.z -= std::cos (camera.p) * camera_speed;
        }

      //////////////////////////////////////////////////////////////////////////////////////////////

      window.clear ({ 0, 0, 0 });

#pragma omp parallel for schedule(guided)
      for (size_t i = 0; i < bodies.size (); ++i)
        {
          const auto &body = bodies[i];

          auto p = project (camera, body.position);

          if (p.z != 0)
            {
              points[i].position.x = p.x;
              points[i].position.y = p.y;

              points[i].color = sf::Color::White;

              auto z_pc = t::spatial_unit::from_Mm (p.z).as_pc ();
              auto z_lu = z_pc / camera.d * 0.5e2;

              auto a = std::clamp (1.0 / (1.0 + std::pow (z_lu, 2.0)), 0.0, 1.0);

              points[i].color.a = a * 255;

              if (seeall)
                points[i].color.a = 64;
            }
          else
            points[i].color.a = 0;
        }

      window.draw (points, sf::BlendMode (sf::BlendAdd));

      {
        auto p = project (camera, t::vector3su::ZERO);

        if (p.z != 0)
          {
            sf::CircleShape earth (5);

            earth.setOrigin ({ (float)5, (float)5 });

            earth.setPosition ({ (float)p.x, (float)p.y });

            earth.setOutlineThickness (2);

            earth.setOutlineColor (sf::Color::Blue);

            earth.setFillColor (sf::Color::Transparent);

            window.draw (earth);
          }
      }

      window.display ();
    }
}

