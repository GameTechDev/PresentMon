// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
import { dispatchDelayedTask, type DelayedTask, awaitDelayedPromise } from "./timing";
import { type Process } from "./process";
import { Api } from "./api"
import { getBlocklist } from "./block-list";

var utilizationPollTask: DelayedTask<Promise<Process|null>>|null = null;

export async function launchAutotargetting(setter: (pid: number)=>void): Promise<void> {
    const top = await doGpuUtilizationTopPolling(250);
    if (top !== null) {
        setter(top.pid);
    }
}

export async function doGpuUtilizationTopPolling(specifiedDelayMs: number): Promise<Process|null> {
    // initial poll without any delay
    let delayMs = 0;
    // loop polling while result not received
    while (true) {
        utilizationPollTask = dispatchDelayedTask(async () => {
            return await Api.getTopGpuProcess(getBlocklist());        
        }, delayMs);
        const result = await awaitDelayedPromise(utilizationPollTask.promise);
        // if result is not null, we have a process to target
        // otherwise, the delayed task was cancelled
        if (result !== null) {
            utilizationPollTask = null;
            return result;
        }
        // null here means somebody called cancelTopPolling
        // probably not necessary, but just in case
        if (utilizationPollTask === null) {
            return result;
        }
        // subsequent polls use the passed in delay
        delayMs = specifiedDelayMs
    }
}

export function cancelTopPolling(): void {
    if (utilizationPollTask !== null) {
        utilizationPollTask.token.cancel();
        utilizationPollTask = null;
    }
}