#pragma once

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <unordered_map>

class Particles {
public:
    using Point = boost::geometry::model::d2::point_xy<float, boost::geometry::cs::cartesian>;
    using PointIndex = std::pair<Point, uint32_t>;
    using RTree = boost::geometry::index::rtree<PointIndex, boost::geometry::index::rstar<16>>;

    struct Particle {
        uint32_t id;
        Point position;
        Point velocity;
    };

    struct Config {
        Point simulation_origin = {0, 0};
        float simulation_radius = 100;
        float travel_speed = 0.67f;
        float neighbor_radius = 5.0f;
        float alpha = M_PI;
        float beta = 17 * M_PI / 180;
        float particle_density = 10;
        size_t particle_lifetime = 200;
    };

    Particles(Config config)
            : _config(std::move(config)) {}

    void update();

    void updateVelocity(Particle& particle, std::pair<int, int> neighbors);

    std::pair<int, int> countLeftRightNeighbors(const Particle& particle) const;

    const std::unordered_map<uint32_t, Particle>& getParticles() const { return _particles; }

private:
    Config _config;
    RTree _rtree;
    std::unordered_map<uint32_t, Particle> _particles;
};
