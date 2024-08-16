// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once
#include "Application.hpp"

namespace Raytracer {
    inline Application& Application::GetInstance() {
        return *m_SInstance;
    }

    inline bool Application::IsRunning() const {
        return m_IsRunning;
    }

    inline void Application::Close() {
        m_IsRunning = false;
    }
}
