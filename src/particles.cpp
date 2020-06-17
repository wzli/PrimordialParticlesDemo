#include <particles.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

#include <stdexcept>

namespace bg = boost::geometry;

Particles::Particles(Config config)
        : _config(std::move(config))
        , _random_generator(_random_device())
        , _normal_distribution(0, _config.simulation_radius * 0.5f) {
    if (_config.simulation_radius <= 0) {
        throw std::invalid_argument("positive simulation radius required");
    }
    if (_config.neighbor_radius <= 0) {
        throw std::invalid_argument("positive neighbor radius required");
    }
    if (_config.simulation_density <= 0) {
        throw std::invalid_argument("positive particle density required");
    }
}

void Particles::update() {
    // simulate each particle
    for (auto& particle : _particles) {
        // update velocity
        updateParticleNeighborCount(particle.second);
        updateParticleVelocity(particle.second);
        // update position
        bg::add_point(particle.second.position, particle.second.velocity);
    }
    // delete particles outside of simulation radius
    for (auto& particle : _particles) {
        auto distance_squared =
                bg::comparable_distance(particle.second.position, _config.simulation_origin);
        if (distance_squared > (_config.simulation_radius * _config.simulation_radius)) {
            _particles.erase(particle.first);
        }
    }
    respawnParticles();
    rebuildRTree();
}

void Particles::respawnParticles() {
    while (_particles.size() < M_PI * _config.simulation_radius * _config.simulation_radius *
                                       _config.simulation_density) {
        auto id = _random_generator();
        while (_particles.find(id) != _particles.end()) {
            id = _random_generator();
        }
        Particle new_particle{id, 0, 0,
                {_normal_distribution(_random_generator), _normal_distribution(_random_generator)},
                {_config.travel_speed, 0}};
        bg::add_point(new_particle.position, _config.simulation_origin);
        _particles[id] = std::move(new_particle);
    }
}

void Particles::rebuildRTree() {
    _rtree.clear();
    _insert_buffer.clear();
    for (const auto& particle : _particles) {
        _insert_buffer.emplace_back(particle.second.position, particle.first);
    }
    _rtree.insert(_insert_buffer.begin(), _insert_buffer.end());
}

void Particles::updateParticleVelocity(Particle& particle) const {
    float rotation = bg::math::sign(particle.right_neighbors - particle.left_neighbors) *
                     _config.beta * (particle.left_neighbors + particle.right_neighbors);
    bg::strategy::transform::rotate_transformer<bg::radian, float, 2, 2> rotation_matrix(rotation);
    bg::transform(particle.velocity, particle.velocity, rotation_matrix);
}

void Particles::updateParticleNeighborCount(Particle& particle) const {
    particle.left_neighbors = 0;
    particle.right_neighbors = 0;
    for (auto query_itr = _rtree.qbegin(bg::index::nearest(particle.position, _rtree.size()));
            query_itr != _rtree.qend(); ++query_itr) {
        auto distance_squared = bg::comparable_distance(particle.position, query_itr->first);
        if (distance_squared > (_config.neighbor_radius * _config.neighbor_radius)) {
            break;
        }
        Point neighbor_direction = query_itr->first;
        bg::subtract_point(neighbor_direction, particle.position);
        if (bg::cross_product(neighbor_direction, particle.velocity).x() > 0) {
            ++particle.left_neighbors;
        } else {
            ++particle.right_neighbors;
        }
    }
}
