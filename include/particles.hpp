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
        Point position;
        Point velocity;
        uint32_t left_neighbors = 0;
        uint32_t right_neighbors = 0;
        uint32_t close_neighbors = 0;
    };

    using ParticleLookup = std::unordered_map<uint32_t, Particle>;

    struct Config {
        Point simulation_origin = {0, 0};
        float simulation_radius = 25;
        float simulation_min_density = 0.08;
        float simulation_max_density = 0.15;
        float travel_speed = 0.67f;
        float neighbor_radius = 5.0f;
        float close_radius = 1.3f;
        float alpha = M_PI;
        float beta = 17 * M_PI / 180;
    };

    Particles(Config config);
    void update();
    void spawnParticle(const Point& position);

    // accesors
    const Config& getConfig() const { return _config; }
    Config& getConfig() { return _config; }

    const ParticleLookup& getParticles() const { return _particles; }
    ParticleLookup& getParticles() { return _particles; }

    const RTree& getRTree() const { return _rtree; }
    RTree& getRTree() { return _rtree; }

private:
    void respawnParticles();
    void pruneParticles();
    void rebuildRTree();
    void updateParticleNeighborCount(Particle& particle) const;
    void updateParticleVelocity(Particle& particle) const;

    Config _config;
    std::random_device _random_device;
    std::mt19937 _random_generator;
    std::uniform_real_distribution<float> _uniform_distribution;
    RTree _rtree;
    std::vector<PointValue> _insert_buffer;
    ParticleLookup _particles;
};
