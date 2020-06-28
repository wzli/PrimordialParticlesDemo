#include <display.hpp>
#include <particles.hpp>
#include <zmq_http_server.hpp>
#include <vsm/zmq_transport.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/program_options.hpp>

#include <cmath>
#include <iostream>
#include <thread>

static constexpr char INDEX_RESPONSE[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
#include <index.html>
        ;

static constexpr char SVG_RESPONSE_HEADER[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/svg+xml\r\n"
        "Content-Encoding: gzip\r\n"
        "\r\n";

static std::stringstream compress(std::stringstream& in) {
    namespace bio = boost::iostreams;
    std::stringstream ss;
    bio::filtering_streambuf<bio::input> out;
    out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
    out.push(in);
    bio::copy(out, ss);
    return ss;
}

int main(int argc, char* argv[]) {
    // parse arguments
    namespace po = boost::program_options;
    po::variables_map args;
    try {
        po::options_description desc("Allowed options");
        // clang-format off
        desc.add_options()
        ("name,n", po::value<std::string>()->default_value("node"), "mesh node name")
        ("address,a", po::value<std::string>()->required(), "mesh node external address")
        ("bootstrap-peer,b", po::value<std::vector<std::string>>(), "mesh bootstrap peer (address:port)")
        ("x-coord,x", po::value<float>()->default_value(0), "mesh node x coordinate")
        ("y-coord,y", po::value<float>()->default_value(0), "mesh node y coordinate")
        ("connection-degree,c", po::value<uint32_t>()->default_value(6), "nearest peer connections")
        ("sim-radius,r", po::value<float>()->default_value(25), "simulation region radius")
        ("sim-density,d", po::value<float>()->default_value(0.08f), "simulation particle density")
        ("mesh-port,P", po::value<uint32_t>()->default_value(11511), "mesh node UDP port")
        ("http-port,p", po::value<uint32_t>()->default_value(8000), "http server TCP port")
        ("sim-interval,i", po::value<uint32_t>()->default_value(20), "sim update interval (ms)")
        ("mesh-interval,I", po::value<uint32_t>()->default_value(500), "mesh update interval (ms)")
        ("message-size,m", po::value<uint32_t>()->default_value(7000), "transmission message size")
        ("verbosity,v", po::value<uint32_t>()->default_value(vsm::Logger::WARN), "verbosity filter 0-6")
        ("help,h", "produce help message");
        // clang-format on
        po::positional_options_description p;
        p.add("address", 1).add("bootstrap-peer", -1);
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), args);
        if (args.count("help")) {
            std::cout << "Usage: " << argv[0];
            std::cout << " [options] address [bootstrap-peer] [bootstrap-peer] ..." << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }
        po::notify(args);
    } catch (const po::error& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    // create config from parsed arguments
    Particles::Config sim_config;
    sim_config.simulation_origin = {args["x-coord"].as<float>(), args["y-coord"].as<float>()};
    sim_config.simulation_radius = args["sim-radius"].as<float>();
    sim_config.simulation_density = args["sim-density"].as<float>();
    auto http_port = std::to_string(args["http-port"].as<uint32_t>());
    auto mesh_port = std::to_string(args["mesh-port"].as<uint32_t>());
    uint32_t sim_interval = args["sim-interval"].as<uint32_t>();
    vsm::MeshNode::Config mesh_config{
            vsm::msecs(args["mesh-interval"].as<uint32_t>()),  // peer update interval
            vsm::msecs(sim_interval * 30),                     // entity expiry interval
            args["message-size"].as<uint32_t>(),               // entity updates size
            {},                                                // ego sphere
            {
                    args["name"].as<std::string>(),                                  // name
                    "udp://" + args["address"].as<std::string>() + ":" + mesh_port,  // address
                    {sim_config.simulation_origin.x(),
                            sim_config.simulation_origin.y()},  // coordinates
                    sim_config.simulation_radius,               // power radius
                    args["connection-degree"].as<uint32_t>(),   // connection_degree
                    200,                                        // lookup size
                    0,                                          // rank decay
            },
            std::make_shared<vsm::ZmqTransport>("udp://*:" + mesh_port),  // transport
            std::make_shared<vsm::Logger>(),                              // logger
    };
    // log to console
    mesh_config.logger->addLogHandler(
            static_cast<vsm::Logger::Level>(args["verbosity"].as<uint32_t>()),
            [&](vsm::msecs time, vsm::Logger::Level level, vsm::Error error, const void*,
                    size_t len) {
                if (error.type == vsm::EgoSphere::ENTITY_UPDATED) {
                    return;
                }
                std::cout << time.count() << " lv: " << level << ", type: " << error.type
                          << ", code: " << error.code << ", msg: " << error.what() << std::endl;
                if (error.type == vsm::MeshNode::MESSAGE_VERIFY_FAIL) {
                    std::cout << "\tdropped buffer size " << len << std::endl;
                }
            });

    // create objects from config
    Particles particles(sim_config);
    vsm::MeshNode mesh_node(mesh_config);
    ZmqHttpServer http_server(http_port.c_str());
    Display display;

    // add bootstrap peers
    if (args.count("bootstrap-peer")) {
        for (const auto& peer : args["bootstrap-peer"].as<std::vector<std::string>>()) {
            mesh_node.getPeerTracker().latchPeer(("udp://" + peer).c_str(), 1);
        }
    }

    // define entity to particle conversion
    const auto read_particles = [&particles](const vsm::EgoSphere::EntityLookup& updates) {
        for (const auto& update : updates) {
            const auto& coords = update.second.entity.coordinates;
            uint32_t id = static_cast<uint32_t>(std::stoul(update.first));
            auto& particle = particles.getParticles()[id] = {{coords[0], coords[1]}, {}, id};
            const auto& data = update.second.entity.data;
            std::memcpy(&particle.velocity, data.data(), data.size());
        }
    };

    // define particle to entity conversion
    std::vector<vsm::EntityT> entities;
    const auto generate_entities = [&]() {
        entities.clear();
        for (const auto& particle : particles.getParticles()) {
            vsm::EntityT entity;
            entity.name = std::to_string(particle.first);
            entity.coordinates = {particle.second.position.x(), particle.second.position.y()};
            entity.filter = vsm::Filter::NEAREST;
            entity.range = sim_config.simulation_radius;
            entity.data.resize(sizeof(Particles::Point));
            entity.expiry = sim_interval * 5;
            std::memcpy(entity.data.data(), &particle.second.velocity, sizeof(Particles::Point));
            entities.emplace_back(std::move(entity));
        }
    };

    // particle sim update timer
    http_server.addTimer(sim_interval, [&](int) {
        mesh_node.readEntities(read_particles);
        particles.update();
        generate_entities();
        mesh_node.updateEntities(entities);
    });

    // default page
    http_server.addRequestHandler("/", [](zmq::message_t) {
        return zmq::message_t(
                const_cast<char*>(INDEX_RESPONSE), sizeof(INDEX_RESPONSE), nullptr, nullptr);
    });

    // spawn particle on click
    http_server.addRequestHandler("/spawn", [&particles](zmq::message_t msg) {
        float x, y;
        if (sscanf(static_cast<const char*>(msg.data()), "GET /spawn?x=%f&y=%f HTTP", &x, &y) ==
                2) {
            Particles::Point position(x, y);
            boost::geometry::subtract_value(position, 0.5f);
            boost::geometry::multiply_value(
                    position, particles.getConfig().simulation_radius * M_SQRT2);
            boost::geometry::add_point(position, particles.getConfig().simulation_origin);
            particles.spawnParticle(position);
        }
        static constexpr char response_data[] = "HTTP/1.1 204 No Content\r\n";
        return zmq::message_t(
                const_cast<char*>(response_data), sizeof(response_data), nullptr, nullptr);
    });

    // generate particle display
    http_server.addRequestHandler("/particles", [&particles, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawParticlesSvg(svg_stream, particles);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    // generate network display
    http_server.addRequestHandler("/network", [&mesh_node, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawNetworkSvg(svg_stream, mesh_node);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    // worker thread runs mesh network
    std::thread mesh_thread([&mesh_node]() {
        while (1) {
            mesh_node.getTransport().poll(vsm::msecs(-1));
        }
    });
    mesh_thread.detach();

    // main thread runs http server and particle sim
    while (1) {
        try {
            http_server.poll();
        } catch (const zmq::error_t& e) {
            puts(e.what());
        }
    }
}
