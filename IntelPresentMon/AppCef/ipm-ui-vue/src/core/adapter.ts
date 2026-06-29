// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface Adapter {
    id: number;
    vendor: string;
    name: string;
}

/** Smallest GPU device id reported by introspection (stable global default). */
export function lowestAdapterId(adapters: readonly Adapter[]): number {
    if (adapters.length === 0) {
        return 0;
    }
    let minId = adapters[0].id;
    for (let i = 1; i < adapters.length; i++) {
        const id = adapters[i].id;
        if (id < minId) {
            minId = id;
        }
    }
    return minId;
}