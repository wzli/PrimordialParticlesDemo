#include <particles.hpp>

#include <boost/geometry.hpp>
#include <boost/geometry/arithmetic/cross_product.hpp>
#include <boost/geometry/strategies/transform/matrix_transformers.hpp>

namespace bg = boost::geometry;

void Particles::update() {
    // simulate each particle
    for (auto& particle : _particles) {
        // update velocity
        updateVelocity(particle.second, countLeftRightNeighbors(particle.second));
        // update position
        bg::add_point(particle.second.position, particle.second.velocity);
        // delete particles outside of simulation radius
        auto distance_squared =
                bg::comparable_distance(particle.second.position, _config.simulation_origin);
        if (distance_squared > (_config.simulation_radius * _config.simulation_radius)) {
            _particles.erase(particle.first);
        }
    }
    // spawn particles to maintain density

    // rebuild rtree
    _rtree.clear();
}

void Particles::updateVelocity(Particle& particle, std::pair<int, int> neighbors) {
    float rotation = bg::math::sign(neighbors.second - neighbors.first) * _config.beta *
                     (neighbors.first + neighbors.second);
    bg::strategy::transform::rotate_transformer<bg::radian, float, 2, 2> rotation_matrix(rotation);
    rotation_matrix.apply(Point(0, 0), particle.velocity);
}

std::pair<int, int> Particles::countLeftRightNeighbors(const Particle& particle) const {
    std::pair<int, int> neighbors(0, 0);
    for (auto query_itr = _rtree.qbegin(bg::index::nearest(particle.position, _rtree.size()));
            query_itr != _rtree.qend(); ++query_itr) {
        auto distance_squared = bg::comparable_distance(particle.position, query_itr->first);
        if (distance_squared > (_config.neighbor_radius * _config.neighbor_radius)) {
            break;
        }
        Point neighbor_direction = query_itr->first;
        bg::subtract_point(neighbor_direction, particle.position);
        if (bg::cross_product(neighbor_direction, particle.velocity).x() > 0) {
            ++neighbors.first;
        } else {
            ++neighbors.second;
        }
    }
    return neighbors;
}
