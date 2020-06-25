#include <display.hpp>
#include <particles.hpp>
#include <zmq_http_server.hpp>
#include <vsm/zmq_transport.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/program_options.hpp>

#include <unistd.h>
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
        ("sim-radius,r", po::value<float>()->default_value(25), "simulation region radius")
        ("sim-density,d", po::value<float>()->default_value(0.08), "simulation particle density")
        ("mesh-port,P", po::value<uint32_t>()->default_value(11511), "mesh node UDP port")
        ("http-port,p", po::value<uint32_t>()->default_value(8000), "http server TCP port")
        ("sim-interval,i", po::value<uint32_t>()->default_value(20), "sim update interval (ms)")
        ("mesh-interval,I", po::value<uint32_t>()->default_value(500), "mesh update interval (ms)")
        ("expiry-interval,E", po::value<uint32_t>()->default_value(1000), "particle expiry interval (ms)")
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
    auto http_port = std::to_string(args["http-port"].as<uint32_t>());
    auto mesh_port = std::to_string(args["mesh-port"].as<uint32_t>());
    std::vector<float> coords{args["x-coord"].as<float>(), args["y-coord"].as<float>()};

    vsm::MeshNode::Config mesh_config{
            vsm::msecs(args["mesh-interval"].as<uint32_t>()),    // peer update interval
            vsm::msecs(args["expiry-interval"].as<uint32_t>()),  // peer update interval
            {},                                                  // ego sphere
            {
                    args["name"].as<std::string>(),                                  // name
                    "udp://" + args["address"].as<std::string>() + ":" + mesh_port,  // address
                    coords,                                                          // coordinates
                    6,    // connection_degree
                    200,  // lookup size
                    0,    // rank decay
            },
            std::make_shared<vsm::ZmqTransport>("udp://*:" + mesh_port),  // transport
            nullptr,                                                      // logger
    };

    // particle sim objects
    Display display;
    Particles::Config sim_config;
    sim_config.simulation_origin = {coords[0], coords[1]};
    sim_config.simulation_radius = args["sim-radius"].as<float>();
    sim_config.simulation_density = args["sim-density"].as<float>();
    Particles particles(sim_config);

    // network objects
    vsm::MeshNode mesh_node(mesh_config);
    ZmqHttpServer http_server(http_port.c_str());

    // add bootstrap peers
    if (args.count("bootstrap-peer")) {
        for (const auto& peer : args["bootstrap-peer"].as<std::vector<std::string>>()) {
            mesh_node.getPeerTracker().latchPeer(("udp://" + peer).c_str(), 1);
        }
    }

    // particle sim update timer
    http_server.addTimer(
            args["sim-interval"].as<uint32_t>(), [&particles](int) { particles.update(); });

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
