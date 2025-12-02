# Agent Guidelines for Server

## Build/Test/Lint Commands
- **Run**: `node index.js` (or `npm run dev` for TypeScript with watch mode)
- **Test**: _No test script currently defined in package.json_
- **Single test**: _No test script currently defined in package.json_
- **Type check**: `npx tsc --noEmit` (if tsconfig.json exists)
- **Install deps**: `npm install`

## Code Style
- **Language**: TypeScript with strict type checking
- **Runtime**: Node.js
- **Imports**: Use ES6 imports (`import x from 'y'`), prefer named exports
- **Formatting**: 2-space indentation, no semicolons
- **Types**: Explicit return types on functions, avoid `any`, use interfaces for objects
- **Naming**: camelCase for variables/functions, PascalCase for classes/types, UPPER_SNAKE_CASE for constants
- **Error handling**: Use try-catch with typed Error objects, avoid throwing strings
- **Async**: Prefer async/await over promises, handle rejections explicitly
- **Comments**: Only add comments when explicitly requested

## Environment
- Development environment uses Nix flakes (see flake.nix)
- Use `direnv` for automatic environment loading
