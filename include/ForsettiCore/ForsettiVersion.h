// Forsetti Framework for Windows
// Copyright (c) 2026 James Daley.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ForsettiCore/SemVer.h"

namespace Forsetti {

/// Current framework version. Mirrors ForsettiVersion.swift.
struct ForsettiVersion final {
    static inline const SemVer current{0, 2, 0};
};

} // namespace Forsetti
