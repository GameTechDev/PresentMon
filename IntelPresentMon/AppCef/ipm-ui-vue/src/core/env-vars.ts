import { Api } from "./api";

export interface EnvVars {
    useDebugBlocklist: boolean;
    enableDevMode: boolean;
};

var vars:EnvVars|null = null;

export async function getEnvVars(): Promise<EnvVars> {
    if (vars === null) {
        vars = await Api.loadEnvVars()
    }
    return vars;
}

export function isDevBuild(): boolean {
    return import.meta.env.DEV
}

export function isDevelopment(): boolean {
    if (isDevBuild()) {
        return true;
    }
    return vars !== null && vars.enableDevMode;
}