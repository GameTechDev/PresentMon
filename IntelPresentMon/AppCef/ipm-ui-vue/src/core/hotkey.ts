// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { type Distinct } from "./meta";

export enum Action {
    ToggleCapture,
    ToggleOverlay,
    CyclePreset,
}

export type KeyCode = Distinct<number, "hotkey:KeyCode">;
export type ModifierCode = Distinct<number, "hotkey:ModifierCode">;

export interface KeyOption {
    code: KeyCode;
    text: string;
}

export interface ModifierOption {
    code: ModifierCode;
    text: string;
}

export interface Combination {
    key: KeyCode;
    modifiers: ModifierCode[];
}

export interface Binding {
    combination: Combination|null;
    action: Action;
}

export function combinationsAreSame(a: Combination, b: Combination): boolean {
    return a.key === b.key && a.modifiers.every(ma => b.modifiers.includes(ma));
}