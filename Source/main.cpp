#include <Raytracer/Application.hpp>

int main() {
    Raytracer::WindowProperties properties{1920, 1080, "Raytracer", false, false};

    const auto app = std::make_unique<Raytracer::Application>(properties);

    app->Run();

    return EXIT_SUCCESS;
}