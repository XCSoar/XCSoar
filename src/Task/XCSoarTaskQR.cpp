// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCSoarTaskQR.hpp"
#include "Deserialiser.hpp"
#include "Serialiser.hpp"
#include "XML/DataNodeXML.hpp"
#include "XML/Node.hpp"
#include "XML/Parser.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "io/BufferedOutputStream.hxx"
#include "io/StringOutputStream.hxx"
#include "util/StringAPI.hxx"

#include <stdexcept>

std::optional<std::string>
EncodeXCSoarTaskQRText(const OrderedTask &task)
try {
  XMLNode root_node = XMLNode::CreateRoot("Task");

  {
    WritableDataNodeXML root(root_node);
    SaveTask(root, task);
  }

  StringOutputStream xml;
  BufferedOutputStream buffered(xml);
  /* No indentation: a QR code has a few kilobytes at most. */
  root_node.Serialise(buffered, false);
  buffered.Flush();

  std::string text{XCSOAR_TASK_QR_PREFIX};
  text += xml.GetValue();
  return text;
} catch (...) {
  return std::nullopt;
}

std::unique_ptr<OrderedTask>
DecodeXCSoarTaskQRText(std::string_view xml,
                       const TaskBehaviour &task_behaviour)
{
  const auto xml_root = XML::ParseString(xml);
  const ConstDataNodeXML root(xml_root);
  if (!StringIsEqual(root.GetName(), "Task"))
    throw std::runtime_error("Invalid task");

  auto task = std::make_unique<OrderedTask>(task_behaviour);
  LoadTask(*task, root, nullptr);
  return task;
}
