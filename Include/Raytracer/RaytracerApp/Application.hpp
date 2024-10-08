// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once


#include <Raytracer/rtpch.hpp>

#include <Raytracer/Core/Window.hpp>

#include <Raytracer/Renderer/VulkanRenderer.hpp>

#include <Raytracer/RaytracerApp/RayQueryRenderer.hpp>

#include <chrono>

namespace Raytracer {
    class Application {
    public:
        Application(const WindowProperties& properties, const DebugLevel& debugLevel);
        ~Application();
        
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        void Run();

        [[nodiscard]] inline bool IsRunning() const;

        [[nodiscard]] static inline Application& GetInstance();

    private:
        static Application* m_SInstance;

        std::chrono::high_resolution_clock::time_point m_CurrentTime;

        bool m_IsRunning = false;

        f64 m_DeltaTime = 0.016;

        std::unique_ptr<Window> m_Window;
        std::unique_ptr<Renderer::VulkanRenderer> m_Renderer;
        std::unique_ptr<RayQueryRenderer> m_RayQueryRenderer;

        Camera m_Camera;
        f32 m_CameraSpeed = 1.f;
        f32 m_MouseSensitivity = 1.f;
        f32 m_Fov = 70.f;

        DeletionQueue m_ApplicationDeletionQueue;
        
        void OnUpdate();
        void OnRender();
        void OnEvent(Event& event);

        inline void Close();

        void CreateUi();

        // -------- Event handlers --------
        void OnWindowClose(const WindowCloseEvent& event);
        void OnMouseMovement(const MouseMovedEvent& event);
        void OnKeyDown(const KeyDownEvent& event);
        void OnKeyUp(const KeyUpEvent& event);
    };
}

#include <Raytracer/RaytracerApp/Application.inl>
