# AppCef Web UI Guide

The AppCef web UI lives under `IntelPresentMon\AppCef\ipm-ui-vue`. It is a Vue and Vite application whose production assets are built before the capture application is packaged or run from staged files.

## Normal CMake Path

The examples use the Windows `py` launcher. On a machine with only
`python.exe`, use `python` wherever `py -3` appears. After installing the Python
requirements, configure and build normally:

```powershell
> py -3 -m pip install -r requirements.txt
> cmake -B build -S . -G "Visual Studio 17 2022"
> cmake --build build --config Debug
```

For Visual Studio 2026 (Experimental), use its distinct binary tree:

```powershell
> cmake -B build-vs2026 -S . -G "Visual Studio 18 2026"
> cmake --build build-vs2026 --config Debug
```

CMake invokes `Scripts\web.py` during configuration. It runs `npm ci` only when
the package lock fingerprint or installed dependency check is stale, then builds the
production web assets. Subsequent configurations reuse the current npm install.

To prepare only the web UI, run:

```powershell
> py -3 Scripts\web.py ensure
```

## Manual Production Build

To rebuild only the web UI, run:

```bat
> pushd IntelPresentMon\AppCef\ipm-ui-vue
> npm ci
> npm run build
> popd
```

This manual workflow remains useful for focused web development. The normal
CMake path performs the equivalent preparation automatically.

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
