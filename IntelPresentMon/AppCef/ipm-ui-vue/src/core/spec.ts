// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Preferences } from "./preferences";
import { type Widget } from "./widget";

export interface Spec {
  pid: number|null;
  preferences: Preferences;
  widgets: Widget[];
}
