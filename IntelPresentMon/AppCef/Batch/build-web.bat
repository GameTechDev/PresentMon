cd ipm-ui-vue

echo "Install NPM Packages..."
call npm ci

echo "Build SPA..."
call npm run build

cd ..