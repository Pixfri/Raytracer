// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

namespace Raytracer {
    inline spdlog::logger* Logger::GetLogger() {
        return m_Logger.get();
    }

    namespace Log {
        template <typename... Args>
        constexpr void RtTrace(Args&&... args) {
            Logger::GetLogger()->trace(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr void RtInfo(Args&&... args) {
            Logger::GetLogger()->info(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr void RtWarn(Args&&... args) {
            Logger::GetLogger()->warn(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr void RtError(Args&&... args) {
            Logger::GetLogger()->error(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr void RtCritical(Args&&... args) {
            Logger::GetLogger()->critical(std::forward<Args>(args)...);
        }

        template <typename... Args>
        constexpr void RtFatal(const ErrorCode errorCode, Args&&... args) {
            std::stringstream hexErrorCodeStream;
            hexErrorCodeStream << std::hex << errorCode.GetFormattedErrorCode();
            Logger::GetLogger()->critical("{0} FATAL ERROR: Code: 0x{1}",
                                          EvaluateErrorCode(errorCode), hexErrorCodeStream.str());
            Logger::GetLogger()->critical(std::forward<Args>(args)...);
            exit(errorCode.GetFormattedErrorCode());
        }
    }
}
