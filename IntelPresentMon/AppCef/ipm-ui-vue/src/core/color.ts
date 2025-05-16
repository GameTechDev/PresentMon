// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface RgbaColor {
    r: number;
    g: number;
    b: number;
    a: number;
}

export function makeCssString(c: RgbaColor) {
    return `rgba(${c.r}, ${c.g}, ${c.b}, ${c.a})`;
}