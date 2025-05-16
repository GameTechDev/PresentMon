// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: MIT
import { compareVersions } from "./signature";
import { type PreferenceFile, signature, migratePreferences as migrate } from "./preferences";


export function migratePreferences(file: PreferenceFile): void {
    if (file.signature.code !== signature.code) {
        throw new Error(`wrong signature code in preferences migration: ${file.signature.code}`);
    }
    if (compareVersions(file.signature.version, signature.version) > 0) {
        throw new Error(`error attempted migration from newer to older version: ${file.signature.version} => ${signature.version}`);
    }
    if (compareVersions(file.signature.version, signature.version) === 0) {
        console.warn(`migrateLoadout called but version up to date ${file.signature.version}`);
        return;
    }
    migrate(file.preferences, file.signature.version);
}