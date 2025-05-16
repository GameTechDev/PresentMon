// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Signature } from "./signature"
import { type Widget } from "./widget"

export interface LoadoutFile {
    signature: Signature
    widgets: Widget[]
}

export const signature: Signature = {
    code: "p2c-cap-load",
    version: "0.13.0"
}
