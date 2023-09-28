// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { Signature, compareVersions } from "./signature";
import { LoadoutFile, signature } from "./loadout";
import { Widget, WidgetType } from "./widget";
import { Readout, migrateReadout } from "./readout";
import { Graph, migrateGraph } from "./graph";


export function migrateLoadout(file: LoadoutFile): void {
    if (file.signature.code !== signature.code) {
        throw new Error('wrong signature code in loadout migration');
    }
    if (compareVersions(file.signature.version, signature.version) > 0) {
        throw new Error('error attempted migration from newer to older version');
    }
    if (compareVersions(file.signature.version, signature.version) === 0) {
        console.warn('migrateLoadout called but version up to date');
        return;
    }
    for (const widget of file.widgets) {
        if (widget.widgetType === WidgetType.Graph) {
            migrateGraph(widget as Graph, file.signature.version);
        }
        else if (widget.widgetType === WidgetType.Readout) {
            migrateReadout(widget as Readout, file.signature.version);
        }
        else {
            console.error('Unrecognized widget type in migrateLoadout');
        }
    }
}