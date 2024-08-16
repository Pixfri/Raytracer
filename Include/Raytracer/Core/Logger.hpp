// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <Raytracer/rtpch.hpp>

#include <spdlog/spdlog.h>

namespace Raytracer {
    /*
     * ErrorCode : The representation of an error code when throwing an error.
     * The first byte in the error code represents the module identifier.
     * The last byte in the error code represents the error number in that module.
     */
    struct ErrorCode {
        u8 ModuleId;
        u8 ErrorNumber;

        [[nodiscard]]
        u16 GetFormattedErrorCode() const {
            return (ModuleId << 8) + ErrorNumber;
        }
    };

    using customLogCallback = std::function<void(const spdlog::level::level_enum& level, const std::string& msg)>;

    class Logger {
        static std::shared_ptr<spdlog::logger> m_Logger;

    public:
        static void Init();
        static inline spdlog::logger* GetLogger();
        static void AddCallback(const customLogCallback& callback);
    };


    namespace Log {
        std::string EvaluateErrorCode(const ErrorCode& errorCode); 
        
        template <typename... Args>
        constexpr void RtTrace(Args&&... args);

        template <typename... Args>
        constexpr void RtInfo(Args&&... args);

        template <typename... Args>
        constexpr void RtWarn(Args&&... args);

        template <typename... Args>
        constexpr void RtError(Args&&... args);

        template <typename... Args>
        constexpr void RtCritical(Args&&... args);

        template <typename... Args>
        constexpr void RtFatal(ErrorCode errorCode, Args&&... args);
    }
}

#include <Raytracer/Core/Logger.inl>
