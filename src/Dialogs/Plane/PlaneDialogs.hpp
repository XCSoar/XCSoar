// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct Plane;

void
dlgPlanesShowModal() noexcept;

bool
dlgPlaneDetailsShowModal(Plane &plane) noexcept;

bool
dlgPlanePolarShowModal(Plane &plane) noexcept;
