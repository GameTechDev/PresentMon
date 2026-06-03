# AppCef Web UI Guide

The AppCef web UI lives under `IntelPresentMon\AppCef\ipm-ui-vue`. It is a Vue and Vite application whose production assets are built before the capture application is packaged or run from staged files.

## Bootstrap Path

From the repository root, run:

```powershell
> .\bootstrap.ps1
```

The bootstrap script enters `IntelPresentMon\AppCef`, runs `Batch\build-web.bat`, installs npm packages with `npm ci`, and builds the production web assets with `npm run build`.

## Manual Production Build

To rebuild only the web UI, run:

```bat
> pushd IntelPresentMon\AppCef\ipm-ui-vue
> npm ci
> npm run build
> popd
```

Run this on a fresh clone, after package changes, or when the production web assets need to be refreshed.

## Development Server

For local web UI development, run the Vite development server instead of the production build:

```bat
> pushd IntelPresentMon\AppCef\ipm-ui-vue
> npm ci
> npm run dev
```

Then start the PresentMon Capture Application with:

```bat
> PresentMon.exe --ui-option url "http://localhost:5173/"
```

This causes the capture application to load web content from the local development server rather than from staged files under `ipm-ui-vue`.

## Useful Commands

```bat
> npm run build
> npm run type-check
> npm run preview
```

`npm run build` runs type checking and the production Vite build. `npm run type-check` runs the Vue TypeScript checks. `npm run preview` serves the production build locally for inspection.
