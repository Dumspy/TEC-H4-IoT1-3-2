# Agent Guidelines for Server

## Build/Test/Lint Commands
- **Run**: `bun run index.ts` (or main entry point)
- **Test**: `bun test` (for files matching `*.test.ts`)
- **Single test**: `bun test <path/to/file.test.ts>`
- **Type check**: `bun run tsc --noEmit` (if tsconfig.json exists)
- **Install deps**: `npm install`

## Code Style
- **Language**: TypeScript with strict type checking
- **Runtime**: Node.js
- **Imports**: Use ES6 imports (`import x from 'y'`), prefer named exports
- **Formatting**: 2-space indentation, no semicolons unless required
- **Types**: Explicit return types on functions, avoid `any`, use interfaces for objects
- **Naming**: camelCase for variables/functions, PascalCase for classes/types, UPPER_SNAKE_CASE for constants
- **Error handling**: Use try-catch with typed Error objects, avoid throwing strings
- **Async**: Prefer async/await over promises, handle rejections explicitly
- **Comments**: Only add comments when explicitly requested

## Environment
- Development environment uses Nix flakes (see flake.nix)
- Use `direnv` for automatic environment loading
