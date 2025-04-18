// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

class WritableDataNode;
class OrderedTask;

void
SaveTask(WritableDataNode &node, const OrderedTask &task);
