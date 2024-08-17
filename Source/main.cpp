#include <Raytracer/RaytracerApp/Application.hpp>

int main() {
    Raytracer::WindowProperties properties{1920, 1080, "Raytracer", false, false};

#if defined(RT_DEBUG)
    constexpr auto debugLevel = Raytracer::DebugLevel::Debug;
#else
    constexpr auto debugLevel = Raytracer::DebugLevel::None;
#endif

    const auto app = std::make_unique<Raytracer::Application>(properties, debugLevel);

    app->Run();

    return EXIT_SUCCESS;
}