// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
export interface Process {
    pid: number;
    name: string;
    windowName: string | null;
}