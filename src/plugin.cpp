// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Ville Väisänen

#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p) {
  pluginInstance = p;
  p->addModel(modelContour);
}
