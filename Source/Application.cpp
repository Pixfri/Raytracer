// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/Application.hpp>

#include <Raytracer/Core/Logger.hpp>

#include <cassert>

namespace Raytracer {
    Application* Application::m_SInstance = nullptr;

    Application::Application(const WindowProperties& properties, const DebugLevel& debugLevel) {
        assert(!m_SInstance && "Only one instance of this application can run at a time.");

        m_SInstance = this;

        Logger::Init();

        m_Window = std::make_unique<Window>(properties);
        m_Window->SetEventCallback([this](Event& event) {
            EventDispatcher dispatcher(event);
            dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_TO_EVENT_HANDLER(Application::OnWindowClose));
            OnEvent(event);
        });

        m_Renderer = std::make_unique<Renderer::VulkanRenderer>(*m_Window, debugLevel);

        m_RaytracingRenderer = std::make_unique<ApplicationRenderer>(m_Renderer.get());        

        m_IsRunning = true;

        Log::RtInfo("Application started.");
    }

    Application::~Application() {
        Log::RtInfo("Quitting application.");

        m_SInstance = nullptr;
    }

    void Application::Run() {
        while (m_IsRunning) {
            m_Window->Update();

            // Compute delta time.
            const auto oldTime = m_CurrentTime;
            m_CurrentTime = std::chrono::high_resolution_clock::now();

            const std::chrono::duration<f64, std::milli> timeSpan = (m_CurrentTime - oldTime);
            m_DeltaTime = timeSpan.count() / 1000.f;

            OnUpdate();
            OnRender();
        }
    }

    void Application::OnUpdate() {
    }

    void Application::OnRender() const {
        const auto commandBuffer = m_Renderer->BeginCommandBuffer(*m_Window);
        
        m_RaytracingRenderer->Raytrace(commandBuffer);
        
        m_Renderer->EndCommandBuffer(*m_Window);
    }

    void Application::OnEvent(Event& event) {
    }

    void Application::OnWindowClose(WindowCloseEvent& event) {
        m_IsRunning = false;
    }
}
