// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of the "Raytracer" project.
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <ranges>

namespace Raytracer {
    // Engine type definitions
    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using usize = u64;

    using f32 = float;
    using f64 = double;

    struct DeletionQueue {
        std::deque<std::function<void()>> Deletors;

        void PushFunction(std::function<void()>&& function) {
            Deletors.push_back(std::move(function));
        }

        void Flush() {
            // Reverse iterate the deletion queue to execute all the functions.
            for (auto& deletor : std::ranges::reverse_view(Deletors)) {
                deletor();
            }

            Deletors.clear();
        }
    };
}