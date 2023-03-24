// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TopographyStore;
class OperationEnvironment;

bool
LoadConfiguredTopography(TopographyStore &store,
                         OperationEnvironment &operation);
