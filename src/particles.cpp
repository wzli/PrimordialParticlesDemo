#include <particles.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

#include <stdexcept>

namespace bg = boost::geometry;

Particles::Particles(Config config)
        : _config(std::move(config))
        , _random_generator(_random_device())
        , _uniform_distribution(-_config.simulation_radius, _config.simulation_radius) {
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

void Particles::update(bool rebuild_rtree) {
    // simulate each particle
    for (auto& particle : _particles) {
        // update velocity
        updateParticleNeighborCount(particle.second);
        updateParticleVelocity(particle.second);
        // update position
        bg::add_point(particle.second.position, particle.second.velocity);
    }
    // delete particles outside of simulation radius
    for (auto particle = _particles.begin(); particle != _particles.end();) {
        auto distance_squared =
                bg::comparable_distance(particle->second.position, _config.simulation_origin);
        if (distance_squared > (_config.simulation_radius * _config.simulation_radius)) {
            particle = _particles.erase(particle);
        } else {
            ++particle;
        }
    }
    respawnParticles();
    if (rebuild_rtree) {
        rebuildRTree();
    }
}

void Particles::spawnParticle(const Point& position, bool insert_rtree) {
    uint32_t id = _random_generator();
    while (_particles.count(id)) {
        id = _random_generator();
    }
    _particles[id] = {position, {_config.travel_speed, 0}, id};
    if (insert_rtree) {
        _rtree.insert(PointValue(position, id));
    }
}

void Particles::respawnParticles() {
    while (_particles.size() < M_PI * _config.simulation_radius * _config.simulation_radius *
                                       _config.simulation_density) {
        Point position(
                _uniform_distribution(_random_generator), _uniform_distribution(_random_generator));
        bg::add_point(position, _config.simulation_origin);
        spawnParticle(position, false);
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
    float rotation =
            _config.alpha +
            _config.beta * (particle.left_neighbors + particle.right_neighbors) *
                    bg::math::sign<int>(particle.left_neighbors - particle.right_neighbors);
    bg::strategy::transform::rotate_transformer<bg::radian, float, 2, 2> rotation_matrix(rotation);
    bg::transform(particle.velocity, particle.velocity, rotation_matrix);
}

void Particles::updateParticleNeighborCount(Particle& particle) const {
    particle.left_neighbors = 0;
    particle.right_neighbors = 0;
    particle.close_neighbors = 0;
    for (auto query_itr = _rtree.qbegin(bg::index::nearest(particle.position, _rtree.size()));
            query_itr != _rtree.qend(); ++query_itr) {
        auto distance_squared = bg::comparable_distance(particle.position, query_itr->first);
        if (distance_squared > (_config.neighbor_radius * _config.neighbor_radius)) {
            break;
        }
        if (distance_squared < (_config.close_radius * _config.close_radius)) {
            ++particle.close_neighbors;
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
