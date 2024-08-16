#include <Raytracer/Core/Logger.hpp>

int main() {
    Raytracer::Logger::Init();

    Raytracer::Log::RtTrace("Logger Test.");
    Raytracer::Log::RtInfo("Logger Test.");
    Raytracer::Log::RtWarn("Logger Test.");
    Raytracer::Log::RtError("Logger Test.");
    Raytracer::Log::RtCritical("Logger Test.");
    Raytracer::Log::RtFatal({0x00, 0x00}, "Logger Test.");

    return 0;
}