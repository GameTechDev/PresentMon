// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface Signature {
    code: "p2c-cap-load"|"p2c-cap-pref",
    version: string,
}

export function isValidVersion(version: string): boolean {
    return /^(\d+\.)?(\d+\.)?(\d+)$/.test(version);
}

// returns +ve if v1 > v2
export function compareVersions(version1: string, version2: string): number {
    if (!isValidVersion(version1) || !isValidVersion(version2)) {
        throw new Error('Invalid version format. Versions must be in the format x.y.z where x, y, and z are integers.');
    }

    const v1parts = version1.split('.').map(Number);
    const v2parts = version2.split('.').map(Number);

    for (let i = 0; i < 3; i++) {
        const v1 = v1parts[i] || 0;
        const v2 = v2parts[i] || 0;

        if (v1 > v2) return 1;
        if (v1 < v2) return -1;
    }

    return 0;
}