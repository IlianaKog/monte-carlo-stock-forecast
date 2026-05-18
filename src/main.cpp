#include "server.h"
#include <iostream>
#include <filesystem>
#include <stdexcept>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); }
        catch (...) {
            std::cerr << "Invalid port argument, using 8080\n";
        }
    }

    // Locate data/ and web/ relative to the executable's working directory.
    // When running from the build/ folder: paths go up one level.
    auto resolve = [](const std::string& rel) -> std::string {
        namespace fs = std::filesystem;
        for (const auto& candidate : {rel, "../" + rel, "../../" + rel}) {
            if (fs::exists(candidate)) return fs::canonical(candidate).string();
        }
        throw std::runtime_error("Cannot locate directory: " + rel);
    };

    try {
        const std::string data_dir = resolve("data");
        const std::string web_dir  = resolve("web");
        ForecastServer server(data_dir, web_dir, port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
