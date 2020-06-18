#pragma once

#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <random>
#include <vector>
#include <unordered_map>

class Particles {
public:
    using Point = boost::geometry::model::d2::point_xy<float, boost::geometry::cs::cartesian>;
    using PointValue = std::pair<Point, uint32_t>;
    using RTree = boost::geometry::index::rtree<PointValue, boost::geometry::index::rstar<16>>;

    struct Particle {
        uint32_t id;
        uint32_t left_neighbors;
        uint32_t right_neighbors;
        uint32_t close_neighbors;
        Point position;
        Point velocity;
    };

    struct Config {
        Point simulation_origin = {0, 0};
        float simulation_radius = 50;
        float simulation_density = 0.12;
        float travel_speed = 0.67f;
        float neighbor_radius = 5.0f;
        float close_radius = 1.3f;
        float alpha = M_PI;
        float beta = 17 * M_PI / 180;
    };

    Particles(Config config);
    void update();

    const Config& getConfig() const { return _config; }
    const RTree& getRTree() const { return _rtree; }
    const std::unordered_map<uint32_t, Particle>& getParticles() const { return _particles; }

private:
    void respawnParticles();
    void rebuildRTree();
    void updateParticleNeighborCount(Particle& particle) const;
    void updateParticleVelocity(Particle& particle) const;

    const Config _config;
    std::random_device _random_device;
    std::mt19937 _random_generator;
    std::uniform_real_distribution<float> _uniform_distribution;
    RTree _rtree;
    std::vector<PointValue> _insert_buffer;
    std::unordered_map<uint32_t, Particle> _particles;
};
