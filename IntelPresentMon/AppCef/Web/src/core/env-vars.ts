import { Api } from "./api";

export interface EnvVars {
    useDebugBlocklist: boolean;
    enableDevMode: boolean;
};

var vars:EnvVars|null = null;

export async function GetEnvVars(): Promise<EnvVars> {
    if (vars === null) {
        vars = await Api.loadEnvVars()
    }
    return vars;
}

export function IsDevelopment(): boolean {
    if (process?.env?.NODE_ENV === 'development') {
        return true;
    }
    return vars !== null && vars.enableDevMode;
}