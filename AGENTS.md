# AGENTS.md - Development Guide for AI Coding Agents

Important Rules:
1. Do not worry about tokens, Do not worry about how long it took you. Do your best to achieve the goal.
2. Do not generate any "Session Continuation". save the tokens for thinking or working.
3. Keep the report short.

This guide provides essential information for AI coding agents working in the tree-sitter-gd repository.

## Project Overview

This is a Godot 4.3 GDExtension that integrates tree-sitter parsing for GDScript using:
- **godot-cpp** (branch: godot-4.3-stable) - C++ bindings for GDExtension
- **tree-sitter** - Incremental parsing library
- **tree-sitter-gdscript** - GDScript grammar implementation

## Build System

### SCons (Primary Build System)

The project uses SCons as the primary build system. An empty `SConstruct` file exists in the root - implementation is TODO.

**Expected build pattern (following godot-cpp conventions):**
```bash
# Build for development
scons target=template_debug platform=<platform>

# Build for release
scons target=template_release platform=<platform>

# Platform options: linux, windows, macos
# Additional options: arch=x86_64, arch=x86_32
```

### Building Submodules

#### godot-cpp
```bash
cd thirdparty/godot-cpp

# Using Makefile wrapper
make linux64  # or windows64, macos
make TARGET=template_release EXTRA_ARGS="custom_api_file=path/to/extension_api.json" linux64

# Direct SCons
scons platform=linux target=template_debug
scons platform=linux target=template_release api_version=4.3
```

#### tree-sitter
```bash
cd thirdparty/tree-sitter

# Build library
make          # builds shared/static libraries
make test     # runs full test suite via cargo xtask
make lint     # cargo fmt check + clippy
```

#### tree-sitter-gdscript
```bash
cd thirdparty/tree-sitter-gdscript

# Generate parser
npm run generate    # or: tree-sitter generate

# Run grammar tests
npm test           # runs corpus tests
tree-sitter test -i 'Test name'  # run specific test

# Run Node binding tests
npm run testNode   # node --test bindings/node/*_test.js
```

## Testing

### Project Tests (TODO)

The `test/` directory structure exists but tests are not yet implemented:
- `test/unit/` - Empty, unit tests TODO
- `test/gdscript_samples/` - Sample GDScript files for testing

### Submodule Tests

#### tree-sitter Grammar Tests
```bash
cd thirdparty/tree-sitter-gdscript

# Full test suite
npm test

# Run specific test by name
tree-sitter test -i 'Return statements'

# Update test expectations
tree-sitter test -u
```

#### tree-sitter Core Tests
```bash
cd thirdparty/tree-sitter

# Full test suite (Rust)
make test              # runs cargo xtask test
cargo xtask test       # direct invocation
cargo xtask test -g test_name  # specific test

# Web binding tests
cd lib/binding_web
npm test               # runs vitest
npm test -- -t 'pattern'  # match specific test name
```

#### godot-cpp Tests
```bash
cd thirdparty/godot-cpp

# Requires Godot editor binary
GODOT=/path/to/godot ./test/run-tests.sh
```

## Code Style

### C++ Code Style (godot-cpp conventions)

**Formatting:** Uses `.clang-format` based on LLVM style (from `thirdparty/godot-cpp/.clang-format`)

Key rules:
- **Indentation:** Tabs (width: 4)
- **Line length:** No limit (ColumnLimit: 0)
- **Braces:** K&R style (attach), required via `InsertBraces: true`
- **Alignment:** Don't align (operators, brackets)
- **Semicolons:** Auto-removed where appropriate (`RemoveSemicolon: true`)
- **Standard:** C++20

**Include order:**
1. Local headers in quotes: `"header.h"`
2. System headers with extension: `<header.h>`
3. System headers without extension: `<iostream>`

**Naming conventions (godot-cpp style):**
- Classes/Structs: PascalCase
- Functions/Methods: snake_case
- Variables: snake_case
- Constants/Macros: UPPER_SNAKE_CASE
- Private members: snake_case (no prefix)

**Example:**
```cpp
#include "register_types.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>

void initialize_ast_manager_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Implementation
}
```

### GDScript Code Style

Based on samples in `test/gdscript_samples/`:

**Typing:**
- Always use type hints for variables: `var health: int = 100`
- Always annotate function parameters and returns: `func take_damage(amount: int) -> void:`
- Use type inference operator when clear: `var direction := Input.get_vector(...)`

**Naming:**
- Functions/Variables: snake_case
- Constants: UPPER_SNAKE_CASE
- Classes: PascalCase
- Private members: `_prefix` (underscore prefix)
- Signals: snake_case

**Structure:**
- `@tool` annotation first if needed
- `extends` statement
- `class_name` declaration
- Signals
- `@export` variables
- Public variables
- Private variables (with `_` prefix)
- `@onready` variables
- Lifecycle methods (`_ready`, `_process`, `_physics_process`)
- Public methods
- Private methods (with `_` prefix)
- Static methods last

**Example:**
```gdscript
@tool
extends CharacterBody2D

class_name Player

signal health_changed(new_value: int)

@export var max_health: int = 100
@export_range(0.0, 1000.0) var speed: float = 300.0

var _health: int = max_health:
	set(value):
		_health = clampi(value, 0, max_health)
		health_changed.emit(_health)

@onready var sprite: Sprite2D = $Sprite2D

func _ready() -> void:
	_health = max_health

func take_damage(amount: int) -> void:
	_health -= amount
	if _health <= 0:
		_die()

func _die() -> void:
	queue_free()
```

### TypeScript/JavaScript (tree-sitter bindings)

**Formatting:**
- **Indentation:** 2 spaces
- **Semicolons:** Required
- **Quotes:** Prefer single quotes
- **Line length:** ~120 characters (from editorconfig)

**Import order:**
1. Constants/FFI modules (`./constants`)
2. Core domain classes (Tree, Node, Language, Parser)
3. Marshal/FFI helpers
4. Utilities

**Naming:**
- Classes/Interfaces/Types: PascalCase
- Functions/Methods: camelCase
- Boolean predicates: `isX`, `hasY` prefix
- Constants: UPPER_SNAKE_CASE
- Private cached fields: `_prefix`
- Callback types: suffix with `Callback`

**Type system:**
- Use explicit interfaces for public APIs
- Use `type` imports where appropriate: `import { type QueryMatch }`
- Annotate return types on public methods
- Use `readonly` for immutable properties

**Error handling:**
- Validate inputs early
- Use `throw new Error('descriptive message')` for failures
- Include context in error messages
- Use try/catch only for operations with fallback paths

**Example:**
```typescript
import { C, INTERNAL } from './constants';
import { Parser } from './parser';

export interface ParseOptions {
	includedRanges?: Range[];
}

export class Language {
	private [0] = 0; // Internal WASM handle

	get isNamed(): boolean {
		return this[0] !== 0;
	}

	static async load(url: string): Promise<Language> {
		const response = await fetch(url);
		if (!response.ok) {
			throw new Error(`Failed to load language: ${response.statusText}`);
		}
		// Implementation
	}
}
```

## Editor Configuration

**From `.editorconfig` (godot-cpp):**
```ini
[*]
charset = utf-8
end_of_line = lf
indent_size = 4
indent_style = tab
insert_final_newline = true
max_line_length = 120
trim_trailing_whitespace = true

[*.py]
indent_style = space

[*.{yml,yaml}]
indent_size = 2
indent_style = space
```

## File Organization

### Source Structure
```
src/
├── register_types.cpp    # GDExtension registration (empty)
├── register_types.h      # Header (empty)
├── ast_manager.cpp       # AST management implementation (empty)
└── ast_manager.h         # AST manager interface (empty)
```

### Output Structure
```
addons/ai_script_plugin/
├── ast.gdextension       # GDExtension manifest (empty)
├── plugin.cfg            # Godot plugin config (empty)
├── plugin.gd             # Plugin entry point (empty)
└── bin/                  # Compiled libraries (.dll, .so, .dylib)
```

### Git Ignore Patterns
Build artifacts ignored:
- `.godot/`
- `*.import`
- `addons/ai_script_plugin/bin/*.{dll,so,dylib}`
- `*.o`, `*.os`, `*.obj`
- `.sconsign.dblite`
- `*.pyc`

## Common Tasks

### Adding a New C++ Class

1. Create header in `src/`: `my_class.h`
2. Create implementation in `src/`: `my_class.cpp`
3. Register in `register_types.cpp`:
   ```cpp
   #include "my_class.h"
   
   void initialize_ast_manager_module(ModuleInitializationLevel p_level) {
       if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
           return;
       }
       ClassDB::register_class<MyClass>();
   }
   ```
4. Build: `scons target=template_debug platform=<platform>`

### Format Code

```bash
# C++ (requires clang-format)
cd thirdparty/godot-cpp
find ../../src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# TypeScript/JavaScript
cd thirdparty/tree-sitter-gdscript
npm run format  # prettier on grammar files

cd thirdparty/tree-sitter/lib/binding_web
npm run lint:fix  # eslint --fix
```

### Running Linters

```bash
# C++ (via godot-cpp pre-commit)
cd thirdparty/godot-cpp
pip install pre-commit
pre-commit run --all-files

# Rust (tree-sitter)
cd thirdparty/tree-sitter
make lint  # runs cargo fmt check + clippy

# JavaScript/TypeScript
cd thirdparty/tree-sitter/lib/binding_web
npm run lint
```

## Architecture Patterns

### GDExtension Entry Points

Following godot-cpp patterns:
```cpp
extern "C" {
GDExtensionBool GDE_EXPORT extension_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) {
    godot::GDExtensionBinding::InitObject init_obj(
        p_get_proc_address, p_library, r_initialization
    );
    
    init_obj.register_initializer(initialize_ast_manager_module);
    init_obj.register_terminator(uninitialize_ast_manager_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    
    return init_obj.init();
}
}
```

### Tree-Sitter WASM Patterns (for reference)

The tree-sitter web binding uses these patterns:
- **Marshal pattern:** Convert JS ↔ WASM buffer before FFI calls
- **Internal handles:** Store WASM pointers in numeric properties `[0]`, `[1]`
- **Finalizers:** Register cleanup for WASM memory
- **Lazy caching:** Cache expensive operations (e.g., `Node.children`)

## CI/CD Notes

### Workflows (in submodules)

- **tree-sitter:** `.github/workflows/ci.yml` runs `make lint` + `make lint-web`
- **tree-sitter-gdscript:** `.github/workflows/test.yml` runs `npm ci` → `npm test`
- **godot-cpp:** `.github/workflows/ci-cmake.yml` builds and runs `./test/run-tests.sh`

### Local CI Reproduction

```bash
# Lint everything
cd thirdparty/tree-sitter && make lint && make lint-web
cd ../tree-sitter-gdscript && npm ci && npm run format -- --check

# Test everything
cd thirdparty/tree-sitter && make test
cd ../tree-sitter-gdscript && npm test
cd ../godot-cpp && GODOT=/path/to/godot ./test/run-tests.sh
```

## Important Notes for AI Agents

1. **Empty files:** Most files in `src/` and `addons/` are currently empty placeholders. The implementation is TODO.

2. **Build system incomplete:** The root `SConstruct` is empty. When implementing, follow godot-cpp build patterns.

3. **No root-level tests:** The `test/unit/` directory is empty. When adding tests, consider using GDScript unit testing or C++ test frameworks compatible with Godot.

4. **Submodule versions:**
   - godot-cpp: branch `godot-4.3-stable`
   - tree-sitter: latest main
   - tree-sitter-gdscript: latest main

5. **Never modify submodules:** Work only in the root-level `src/`, `test/`, and `addons/` directories unless explicitly requested.

6. **Type safety:** Always use explicit types in GDScript. The codebase samples demonstrate strong typing conventions.

7. **Error handling:** Validate inputs early and provide clear error messages. Follow godot-cpp error macro patterns (`ERR_FAIL_COND`, `ERR_FAIL_V`).

8. **Memory management:** Use godot-cpp RAII patterns. Avoid manual `new`/`delete` for Godot objects.

9. **Threading:** GDExtension code must be thread-safe. Use Godot's threading primitives if needed.

10. **Platform compatibility:** Code must build on Linux, Windows, and macOS. Use godot-cpp cross-platform types and functions.

## Quick Reference Commands

```bash
# Build (TODO - when SConstruct is implemented)
scons target=template_debug platform=linux

# Format
clang-format -i src/*.cpp src/*.h

# Lint (via submodules)
cd thirdparty/godot-cpp && pre-commit run --all-files

# Test grammar
cd thirdparty/tree-sitter-gdscript && npm test

# Update submodules
git submodule update --init --recursive
```
