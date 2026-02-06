#ifndef AST_MANAGER_H
#define AST_MANAGER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <tree_sitter/api.h>

#define AST_MANAGER_VERSION "0.1.0"

extern "C" const TSLanguage *tree_sitter_gdscript();

using namespace godot;

struct FileState {
	PackedByteArray source_bytes;
	TSTree *tree = nullptr;
};

class ASTManager : public RefCounted {
	GDCLASS(ASTManager, RefCounted)

private:
	TSParser *parser;
	HashMap<String, FileState> open_files;

protected:
	static void _bind_methods();

public:
	ASTManager();
	~ASTManager();

	String ping();
	String get_version();
	Dictionary parse_test(const String &source_code);

	Dictionary open_file(const String &file_path, const String &content);
	bool close_file(const String &file_path);
	Dictionary update_file(const String &file_path, const String &new_content);
	bool is_file_open(const String &file_path);
	PackedStringArray get_open_files();
	String get_file_source(const String &file_path);

	Dictionary query(const String &file_path, const String &query_string);
	String get_node_text(const String &file_path, int start_byte, int end_byte);
	String get_sexp(const String &file_path);

	Dictionary apply_text_edits(const String &file_path, const TypedArray<Dictionary> &edits, bool dry_run);
	Dictionary apply_node_edits(const String &file_path, const TypedArray<Dictionary> &edits, const Dictionary &options);

	String generate_diff(const String &old_text, const String &new_text, const String &file_name);
	Dictionary validate(const String &source_code);
};

#endif // AST_MANAGER_H
