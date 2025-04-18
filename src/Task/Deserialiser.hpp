// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

class ConstDataNode;
class Waypoints;
class OrderedTask;

void
LoadTask(OrderedTask &task, const ConstDataNode &node,
         const Waypoints *waypoints=nullptr);
