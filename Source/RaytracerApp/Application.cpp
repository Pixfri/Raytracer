// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#include <Raytracer/RaytracerApp/Application.hpp>

#include <Raytracer/Core/Logger.hpp>

#include <imgui.h>

#include <cassert>

namespace Raytracer {
    Application* Application::m_SInstance = nullptr;

    Application::Application(const WindowProperties& properties, const DebugLevel& debugLevel) {
        assert(!m_SInstance && "Only one instance of this application can run at a time.");

        m_SInstance = this;

        Logger::Init();
        
        m_Camera.Velocity = glm::vec3(0.f);
        m_Camera.Position = glm::vec3(0.f, 0.f, -2.5f);

        m_Camera.Pitch = 0;
        m_Camera.Yaw = 0;

        m_Window = std::make_unique<Window>(properties);
        m_Window->SetEventCallback([this](Event& event) {
            EventDispatcher dispatcher(event);
            dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_TO_EVENT_HANDLER(Application::OnWindowClose));
            OnEvent(event);
        });

        m_Renderer = std::make_unique<Renderer::VulkanRenderer>(*m_Window, debugLevel);

        m_RayQueryRenderer = std::make_unique<RayQueryRenderer>(m_Renderer.get(), m_Camera);

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
        m_Camera.Update();

        if (m_Fov != m_Camera.Fov) {
            m_Camera.Fov = m_Fov;
            m_Camera.Updated = true;
        }
    }

    void Application::OnRender() {
        m_Renderer->BeginUi();

        CreateUi();
        
        const auto commandBuffer = m_Renderer->BeginCommandBuffer(*m_Window);

        m_Renderer->EndCommandBuffer(*m_Window);
    }

    void Application::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_TO_EVENT_HANDLER(Application::OnMouseMovement));
        dispatcher.Dispatch<KeyDownEvent>(BIND_EVENT_TO_EVENT_HANDLER(Application::OnKeyDown));
        dispatcher.Dispatch<KeyUpEvent>(BIND_EVENT_TO_EVENT_HANDLER(Application::OnKeyUp));
    }

    void Application::CreateUi() {
        if (ImGui::Begin("Camera")) {
            ImGui::SliderFloat("Camera speed", &m_CameraSpeed, 0.1f, 10.f);
            ImGui::SliderFloat("Mouse sensitivity", &m_MouseSensitivity, 0.1f, 10.f);
            ImGui::SliderFloat("Camera FOV", &m_Fov, 45.f, 90.f);
        }
        ImGui::End();
    }

    void Application::OnWindowClose(const WindowCloseEvent& event) {
        (void)event;

        m_IsRunning = false;
    }

    void Application::OnMouseMovement(const MouseMovedEvent& event) {
        m_Camera.OnMouseMovement(event.GetX(), event.GetY(), 0.1f, m_DeltaTime);
    }

    void Application::OnKeyDown(const KeyDownEvent& event) {
        m_Camera.OnKeyDown(event.GetScancode(), m_DeltaTime, 3);
    }

    void Application::OnKeyUp(const KeyUpEvent& event) {
        m_Camera.OnKeyUp(event.GetScancode());
    }

}
