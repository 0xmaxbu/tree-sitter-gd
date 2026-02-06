# GDScript Node Types Reference

This document lists confirmed node type names from the tree-sitter-gdscript grammar, verified by parsing actual GDScript code.

## Verified Node Types (from simple.gd)

### Source Root
- `source` - Root node of the syntax tree

### Statements
- `extends_statement` - Class inheritance declaration
  - Contains: `type` → `identifier`

- `variable_statement` - Variable declaration without @export
  - Fields: `name: (name)`, `type: (type)`, `value: (expression)`

- `export_variable_statement` - Variable with @export annotation
  - Fields: `name: (name)`, `type: (type)`, `value: (expression)`

- `onready_variable_statement` - Variable with @onready annotation
  - Fields: `name: (name)`, `type: (type)`, `value: (expression)`

### Functions
- `function_definition` - Function declaration
  - Fields: 
    - `name: (name)` - Function name (NOT `identifier` in this context)
    - `parameters: (parameters)` - Parameter list
    - `return_type: (type)` - Optional return type annotation
    - `body: (block)` - Function body

### Parameters
- `parameters` - Container for function parameters
- `typed_parameter` - Parameter with type annotation
  - Fields: `name: (identifier)`, `type: (type)`
- `default_parameter` - Parameter with default value
  - Fields: `name: (identifier)`, `value: (expression)`

### Types
- `type` - Type annotation
  - Contains: `identifier` (type name)
- `identifier` - Type name or variable reference

### Names
- `name` - Variable or function name in declaration context
  - NOTE: Function names use `name`, not `identifier` in `function_definition`

### Expressions
- `block` - Code block (function body, if statement body, etc.)
- `assignment` - Assignment expression
  - Fields: `left: (expression)`, `right: (expression)`
- `binary_operator` - Binary operations (+, -, *, /, <=, etc.)
  - Fields: `left: (expression)`, `operator: (string)`, `right: (expression)`
- `call` - Function call
  - Fields: `function: (expression)`, `arguments: (argument_list)`
- `integer` - Integer literal
- `float` - Floating point literal

### Classes
- `class_definition` - Class declaration
  - Fields: `name: (name)`, `body: (block)`
- `class_name_statement` - `class_name` declaration
  - Fields: `name: (name)`

### Annotations
- `annotation` - Decorator/annotation (e.g., @export, @onready, @tool)
  - Contains: `identifier` (annotation name)
- `annotations` - Container for multiple annotations

### Control Flow
- `if_statement` - If/elif/else statement
  - Fields: `condition: (expression)`, `consequence: (block)`, `alternative: (block)`
- `while_statement` - While loop
- `for_statement` - For loop
- `match_statement` - Match/case statement
- `return_statement` - Return statement
  - Fields: `value: (expression)` (optional)

### Other
- `comment` - Single-line comment (#)
- `string` - String literal
- `true`, `false` - Boolean literals
- `null` - Null literal

## Example Query Patterns

### Match all function definitions by name:
```scheme
(function_definition name: (name) @fn_name)
```

### Match all variables (including @export):
```scheme
[
  (variable_statement name: (name) @var_name)
  (export_variable_statement name: (name) @var_name)
]
```

### Match all @export annotations:
```scheme
(annotation (identifier) @anno_name)
```

### Match class declarations:
```scheme
(class_definition name: (name) @class_name)
```

### Match function calls:
```scheme
(call function: (identifier) @func_name)
```

### Match all typed parameters:
```scheme
(typed_parameter name: (identifier) @param_name type: (type) @param_type)
```

## Important Notes

1. **Function names use `name`, not `identifier`:**
   - ✅ Correct: `(function_definition name: (name) @fn_name)`
   - ❌ Wrong: `(function_definition name: (identifier) @fn_name)`

2. **Variable names also use `name`:**
   - `(variable_statement name: (name) @var_name)`

3. **Annotations contain `identifier`:**
   - `(annotation (identifier) @anno_name)` matches "export", "onready", etc.

4. **Type annotations:**
   - Types are wrapped in `type` nodes containing an `identifier`
   - Example: `type: (type (identifier))` matches `-> void`, `: int`, etc.

5. **S-expression format:**
   - Root always starts with `(source ...)`
   - Named fields use `field: (node)` syntax
   - Anonymous nodes appear without field names

## Reference

- Grammar source: `thirdparty/tree-sitter-gdscript/grammar.js`
- Node type definitions: `thirdparty/tree-sitter-gdscript/src/node-types.json`
- Test corpus: `thirdparty/tree-sitter-gdscript/test/corpus/`
- Verified by parsing: `test/gdscript_samples/simple.gd`
