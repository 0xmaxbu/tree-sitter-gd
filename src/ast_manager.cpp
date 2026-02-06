#include "ast_manager.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <sstream>
#include <vector>
#include <functional>
#include "../thirdparty/dtl/dtl.hpp"

static int count_descendants(TSNode node) {
	int count = 0;
	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++) {
		TSNode child = ts_node_child(node, i);
		count++;
		count += count_descendants(child);
	}
	return count;
}

static int count_all_descendants(TSNode node) {
	int count = 1;
	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++) {
		count += count_all_descendants(ts_node_child(node, i));
	}
	return count;
}

static void collect_error_nodes(TSNode node, Array &errors) {
	if (ts_node_is_error(node) || ts_node_is_missing(node)) {
		Dictionary err;
		err["start_byte"] = (int)ts_node_start_byte(node);
		err["end_byte"] = (int)ts_node_end_byte(node);
		TSPoint start = ts_node_start_point(node);
		TSPoint end = ts_node_end_point(node);
		err["start_row"] = (int)start.row;
		err["start_col"] = (int)start.column;
		err["end_row"] = (int)end.row;
		err["end_col"] = (int)end.column;
		errors.push_back(err);
	}

	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++) {
		collect_error_nodes(ts_node_child(node, i), errors);
	}
}

static Dictionary make_parse_result_dict(const String &file_path, TSTree *tree) {
	Dictionary result;
	result["success"] = true;
	result["file_path"] = file_path;

	if (!tree) {
		result["success"] = false;
		result["error"] = "Failed to parse";
		return result;
	}

	TSNode root = ts_tree_root_node(tree);
	bool has_error = ts_node_has_error(root);
	result["has_error"] = has_error;
	result["node_count"] = count_all_descendants(root);

	Array error_ranges;
	int error_count = 0;
	if (has_error) {
		collect_error_nodes(root, error_ranges);
		error_count = error_ranges.size();
	}
	result["error_count"] = error_count;
	result["error_ranges"] = error_ranges;

	return result;
}

ASTManager::ASTManager() {
	parser = ts_parser_new();
	const TSLanguage *lang = tree_sitter_gdscript();
	ts_parser_set_language(parser, lang);
}

ASTManager::~ASTManager() {
	for (const KeyValue<String, FileState> &kv : open_files) {
		if (kv.value.tree) {
			ts_tree_delete(kv.value.tree);
		}
	}
	open_files.clear();

	if (parser) {
		ts_parser_delete(parser);
		parser = nullptr;
	}
}

String ASTManager::ping() {
	return "pong";
}

String ASTManager::get_version() {
	return AST_MANAGER_VERSION;
}

Dictionary ASTManager::parse_test(const String &source_code) {
	Dictionary result;
	result["success"] = false;
	result["root_kind"] = "";
	result["node_count"] = 0;
	result["has_error"] = false;
	result["sexp"] = "";

	if (!parser) {
		return result;
	}

	CharString utf8 = source_code.utf8();
	const char *code_str = utf8.get_data();
	uint32_t code_len = utf8.length();

	TSTree *tree = ts_parser_parse_string(parser, nullptr, code_str, code_len);
	if (!tree) {
		return result;
	}

	result["success"] = true;

	TSNode root = ts_tree_root_node(tree);
	const char *kind_str = ts_node_type(root);
	result["root_kind"] = String(kind_str);

	result["node_count"] = count_descendants(root);
	result["has_error"] = ts_node_has_error(root);

	char *sexp_str = ts_node_string(root);
	if (sexp_str) {
		result["sexp"] = String(sexp_str);
		::free(sexp_str);
	}

	ts_tree_delete(tree);

	return result;
}

Dictionary ASTManager::open_file(const String &file_path, const String &content) {
	CharString utf8 = content.utf8();
	const char *code_str = utf8.get_data();
	uint32_t code_len = utf8.length();

	TSTree *tree = ts_parser_parse_string(parser, nullptr, code_str, code_len);
	if (!tree) {
		Dictionary err;
		err["success"] = false;
		err["error"] = "Failed to create parse tree";
		err["file_path"] = file_path;
		return err;
	}

	if (open_files.has(file_path)) {
		FileState &old_state = open_files[file_path];
		if (old_state.tree) {
			ts_tree_delete(old_state.tree);
		}
	}

	FileState new_state;
	new_state.source_bytes.resize(code_len);
	for (uint32_t i = 0; i < code_len; i++) {
		new_state.source_bytes[i] = static_cast<uint8_t>(code_str[i]);
	}
	new_state.tree = tree;
	open_files.insert(file_path, new_state);

	return make_parse_result_dict(file_path, tree);
}

bool ASTManager::close_file(const String &file_path) {
	if (!open_files.has(file_path)) {
		return false;
	}

	FileState &state = open_files[file_path];
	if (state.tree) {
		ts_tree_delete(state.tree);
		state.tree = nullptr;
	}

	open_files.erase(file_path);
	return true;
}

Dictionary ASTManager::update_file(const String &file_path, const String &new_content) {
	if (!open_files.has(file_path)) {
		Dictionary err;
		err["success"] = false;
		err["error"] = "File not open: " + file_path;
		return err;
	}

	FileState &state = open_files[file_path];
	if (state.tree) {
		ts_tree_delete(state.tree);
		state.tree = nullptr;
	}

	CharString utf8 = new_content.utf8();
	const char *code_str = utf8.get_data();
	uint32_t code_len = utf8.length();

	TSTree *tree = ts_parser_parse_string(parser, nullptr, code_str, code_len);
	if (!tree) {
		Dictionary err;
		err["success"] = false;
		err["error"] = "Failed to parse updated content";
		err["file_path"] = file_path;
		return err;
	}

	state.source_bytes.resize(code_len);
	for (uint32_t i = 0; i < code_len; i++) {
		state.source_bytes[i] = static_cast<uint8_t>(code_str[i]);
	}
	state.tree = tree;

	return make_parse_result_dict(file_path, tree);
}

bool ASTManager::is_file_open(const String &file_path) {
	return open_files.has(file_path);
}

PackedStringArray ASTManager::get_open_files() {
	PackedStringArray result;
	for (const KeyValue<String, FileState> &kv : open_files) {
		result.push_back(kv.key);
	}
	return result;
}

String ASTManager::get_file_source(const String &file_path) {
	if (!open_files.has(file_path)) {
		return "";
	}
	const FileState &state = open_files[file_path];
	return String::utf8(reinterpret_cast<const char *>(state.source_bytes.ptr()), state.source_bytes.size());
}

Dictionary ASTManager::query(const String &file_path, const String &query_string) {
	Dictionary result;
	result["success"] = false;
	result["error"] = "";
	result["matches"] = Array();

	if (!open_files.has(file_path)) {
		result["error"] = "File not open: " + file_path;
		return result;
	}

	const FileState &state = open_files[file_path];
	if (!state.tree) {
		result["error"] = "No tree available for file: " + file_path;
		return result;
	}

	const TSLanguage *lang = tree_sitter_gdscript();
	CharString query_utf8 = query_string.utf8();
	const char *query_str = query_utf8.get_data();
	uint32_t query_len = query_utf8.length();

	uint32_t error_offset = 0;
	TSQueryError error_type = TSQueryErrorNone;
	TSQuery *query = ts_query_new(lang, query_str, query_len, &error_offset, &error_type);

	if (!query) {
		String error_msg = "Query error at offset " + String::num_int64(error_offset) + ": ";
		switch (error_type) {
			case TSQueryErrorSyntax:
				error_msg += "Invalid syntax";
				break;
			case TSQueryErrorNodeType:
				error_msg += "Invalid node type";
				break;
			case TSQueryErrorField:
				error_msg += "Invalid field name";
				break;
			case TSQueryErrorCapture:
				error_msg += "Invalid capture name";
				break;
			case TSQueryErrorStructure:
				error_msg += "Impossible pattern structure";
				break;
			case TSQueryErrorLanguage:
				error_msg += "Language mismatch";
				break;
			default:
				error_msg += "Unknown error";
				break;
		}
		result["error"] = error_msg;
		return result;
	}

	TSQueryCursor *cursor = ts_query_cursor_new();
	if (!cursor) {
		ts_query_delete(query);
		result["error"] = "Failed to create query cursor";
		return result;
	}

	TSNode root_node = ts_tree_root_node(state.tree);
	ts_query_cursor_exec(cursor, query, root_node);

	Array matches;
	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match)) {
		Dictionary match_dict;
		match_dict["pattern_index"] = (int)match.pattern_index;

		Array captures;
		for (uint16_t i = 0; i < match.capture_count; i++) {
			TSQueryCapture capture = match.captures[i];
			TSNode node = capture.node;

			uint32_t capture_name_len = 0;
			const char *capture_name = ts_query_capture_name_for_id(query, capture.index, &capture_name_len);

			uint32_t start_byte = ts_node_start_byte(node);
			uint32_t end_byte = ts_node_end_byte(node);
			TSPoint start_point = ts_node_start_point(node);
			TSPoint end_point = ts_node_end_point(node);

			String text = "";
			if (start_byte < state.source_bytes.size() && end_byte <= state.source_bytes.size()) {
				uint32_t text_len = end_byte - start_byte;
				const char *text_start = reinterpret_cast<const char *>(state.source_bytes.ptr()) + start_byte;
				text = String::utf8(text_start, text_len);
			}

			Dictionary capture_dict;
			capture_dict["name"] = String::utf8(capture_name, capture_name_len);
			capture_dict["node_kind"] = String(ts_node_type(node));
			capture_dict["text"] = text;
			capture_dict["start_byte"] = (int)start_byte;
			capture_dict["end_byte"] = (int)end_byte;
			capture_dict["start_row"] = (int)start_point.row;
			capture_dict["start_col"] = (int)start_point.column;
			capture_dict["end_row"] = (int)end_point.row;
			capture_dict["end_col"] = (int)end_point.column;

			captures.push_back(capture_dict);
		}

		match_dict["captures"] = captures;
		matches.push_back(match_dict);
	}

	ts_query_cursor_delete(cursor);
	ts_query_delete(query);

	result["success"] = true;
	result["matches"] = matches;
	return result;
}

String ASTManager::get_node_text(const String &file_path, int start_byte, int end_byte) {
	if (!open_files.has(file_path)) {
		return "";
	}

	const FileState &state = open_files[file_path];
	if (start_byte < 0 || end_byte < 0 || start_byte > end_byte) {
		return "";
	}

	uint32_t start = static_cast<uint32_t>(start_byte);
	uint32_t end = static_cast<uint32_t>(end_byte);

	if (start >= state.source_bytes.size() || end > state.source_bytes.size()) {
		return "";
	}

	uint32_t length = end - start;
	const char *text_start = reinterpret_cast<const char *>(state.source_bytes.ptr()) + start;
	return String::utf8(text_start, length);
}

String ASTManager::get_sexp(const String &file_path) {
	if (!open_files.has(file_path)) {
		return "";
	}

	const FileState &state = open_files[file_path];
	if (!state.tree) {
		return "";
	}

	TSNode root_node = ts_tree_root_node(state.tree);
	char *sexp_str = ts_node_string(root_node);
	if (!sexp_str) {
		return "";
	}

	String result = String(sexp_str);
	::free(sexp_str);
	return result;
}

static bool edits_overlap(int start1, int end1, int start2, int end2) {
	return end1 > start2;
}

static PackedByteArray apply_single_edit_to_bytes(
	const PackedByteArray &original, 
	int start_byte, 
	int end_byte, 
	const String &new_text
) {
	CharString new_text_utf8 = new_text.utf8();
	const char *new_text_data = new_text_utf8.get_data();
	uint32_t new_text_len = new_text_utf8.length();
	
	PackedByteArray result;
	result.resize(start_byte + new_text_len + (original.size() - end_byte));
	
	for (int i = 0; i < start_byte; i++) {
		result[i] = original[i];
	}
	
	for (uint32_t i = 0; i < new_text_len; i++) {
		result[start_byte + i] = static_cast<uint8_t>(new_text_data[i]);
	}
	
	for (uint32_t i = end_byte; i < original.size(); i++) {
		result[start_byte + new_text_len + (i - end_byte)] = original[i];
	}
	
	return result;
}

Dictionary ASTManager::apply_text_edits(const String &file_path, const TypedArray<Dictionary> &edits, bool dry_run) {
	Dictionary result;
	result["success"] = false;
	result["error"] = "";
	result["new_source"] = "";
	result["has_error"] = false;
	result["error_count"] = 0;
	result["edits_applied"] = 0;

	if (!open_files.has(file_path)) {
		result["error"] = "File not open: " + file_path;
		return result;
	}

	FileState &state = open_files[file_path];
	uint32_t source_length = state.source_bytes.size();

	if (dry_run) {
		result["old_source"] = String::utf8(reinterpret_cast<const char *>(state.source_bytes.ptr()), source_length);
	}

	struct EditInfo {
		int start_byte;
		int end_byte;
		String new_text;
	};

	struct EditInfoComparator {
		bool operator()(const EditInfo &a, const EditInfo &b) const {
			return a.start_byte < b.start_byte;
		}
	};

	Vector<EditInfo> validated_edits;
	validated_edits.resize(edits.size());

	for (int i = 0; i < edits.size(); i++) {
		Dictionary edit_dict = edits[i];
		
		if (!edit_dict.has("start_byte") || !edit_dict.has("end_byte") || !edit_dict.has("new_text")) {
			result["error"] = "Edit " + String::num_int64(i) + " missing required fields";
			return result;
		}

		EditInfo &edit = validated_edits.write[i];
		edit.start_byte = edit_dict["start_byte"];
		edit.end_byte = edit_dict["end_byte"];
		edit.new_text = edit_dict["new_text"];

		if (edit.start_byte < 0) {
			result["error"] = "Edit " + String::num_int64(i) + " has negative start_byte";
			return result;
		}
		if (edit.end_byte < 0) {
			result["error"] = "Edit " + String::num_int64(i) + " has negative end_byte";
			return result;
		}
		if (edit.start_byte > edit.end_byte) {
			result["error"] = "Edit " + String::num_int64(i) + " has start_byte > end_byte";
			return result;
		}
		if (static_cast<uint32_t>(edit.end_byte) > source_length) {
			result["error"] = "Edit " + String::num_int64(i) + " end_byte exceeds source length";
			return result;
		}
	}

	validated_edits.sort_custom<EditInfoComparator>();

	for (int i = 0; i < validated_edits.size() - 1; i++) {
		const EditInfo &edit1 = validated_edits[i];
		const EditInfo &edit2 = validated_edits[i + 1];
		
		if (edits_overlap(edit1.start_byte, edit1.end_byte, edit2.start_byte, edit2.end_byte)) {
			result["error"] = "Edits overlap: edit at byte " + String::num_int64(edit1.start_byte) + 
				" and edit at byte " + String::num_int64(edit2.start_byte);
			return result;
		}
	}

	PackedByteArray modified_bytes = state.source_bytes;
	
	for (int i = validated_edits.size() - 1; i >= 0; i--) {
		const EditInfo &edit = validated_edits[i];
		modified_bytes = apply_single_edit_to_bytes(modified_bytes, edit.start_byte, edit.end_byte, edit.new_text);
	}

	String new_source = String::utf8(reinterpret_cast<const char *>(modified_bytes.ptr()), modified_bytes.size());
	result["new_source"] = new_source;

	const char *parse_data = reinterpret_cast<const char *>(modified_bytes.ptr());
	uint32_t parse_len = modified_bytes.size();
	
	TSTree *new_tree = ts_parser_parse_string(parser, nullptr, parse_data, parse_len);
	if (!new_tree) {
		result["error"] = "Failed to parse after edits";
		return result;
	}

	TSNode root = ts_tree_root_node(new_tree);
	bool has_error = ts_node_has_error(root);
	result["has_error"] = has_error;
	
	Array error_ranges;
	if (has_error) {
		collect_error_nodes(root, error_ranges);
	}
	result["error_count"] = error_ranges.size();

	if (!dry_run) {
		if (state.tree) {
			ts_tree_delete(state.tree);
		}
		state.source_bytes = modified_bytes;
		state.tree = new_tree;
	} else {
		ts_tree_delete(new_tree);
	}

	result["success"] = true;
	result["edits_applied"] = edits.size();
	return result;
}

Dictionary ASTManager::apply_node_edits(const String &file_path, const TypedArray<Dictionary> &edits, const Dictionary &options) {
	Dictionary result;
	result["success"] = false;
	result["error"] = "";
	result["new_source"] = "";
	result["has_error"] = false;
	result["error_count"] = 0;
	result["edits_applied"] = 0;

	if (!open_files.has(file_path)) {
		result["error"] = "File not open: " + file_path;
		return result;
	}

	bool dry_run = options.get("dry_run", false);
	bool auto_indent = options.get("auto_indent", true);
	bool fail_on_parse_error = options.get("fail_on_parse_error", false);

	FileState &state = open_files[file_path];
	uint32_t source_length = state.source_bytes.size();
	String source = String::utf8(reinterpret_cast<const char *>(state.source_bytes.ptr()), source_length);

	struct MatchInfo {
		int edit_index;
		int match_start;
		int match_end;
		String old_text;
		String new_text;
		String node_kind;
	};

	Vector<MatchInfo> matches;
	matches.resize(edits.size());

	for (int i = 0; i < edits.size(); i++) {
		Dictionary edit_dict = edits[i];

		if (!edit_dict.has("old_text") || !edit_dict.has("new_text")) {
			result["error"] = "Edit #" + String::num_int64(i) + ": missing required fields";
			return result;
		}

		String old_text = edit_dict["old_text"];
		String new_text = edit_dict["new_text"];
		String node_kind = edit_dict.get("node_kind", "");

		int match_start = source.find(old_text);
		if (match_start == -1) {
			result["error"] = "Edit #" + String::num_int64(i) + ": old_text not found in source";
			return result;
		}

		int second_match = source.find(old_text, match_start + 1);
		if (second_match != -1) {
			int match_count = 2;
			int search_pos = second_match + 1;
			while (source.find(old_text, search_pos) != -1) {
				match_count++;
				search_pos = source.find(old_text, search_pos) + 1;
			}
			result["error"] = "Edit #" + String::num_int64(i) + ": old_text matches " + String::num_int64(match_count) + " locations, must be unique";
			return result;
		}

		int match_end = match_start + old_text.length();

		if (!node_kind.is_empty()) {
			CharString old_text_utf8 = old_text.utf8();
			int match_start_byte = source.substr(0, match_start).utf8().length();
			int match_end_byte = match_start_byte + old_text_utf8.length();

			TSNode root = ts_tree_root_node(state.tree);
			TSNode covering_node = ts_node_descendant_for_byte_range(root, match_start_byte, match_end_byte - 1);

			bool found_kind = false;
			TSNode current = covering_node;
			while (!ts_node_is_null(current)) {
				const char *node_type = ts_node_type(current);
				if (String(node_type) == node_kind) {
					found_kind = true;
					break;
				}
				current = ts_node_parent(current);
			}

			if (!found_kind) {
				const char *actual_type = ts_node_type(covering_node);
				result["error"] = "Edit #" + String::num_int64(i) + ": matched text is inside '" + String(actual_type) + "', expected '" + node_kind + "'";
				return result;
			}
		}

		MatchInfo &match = matches.write[i];
		match.edit_index = i;
		match.match_start = match_start;
		match.match_end = match_end;
		match.old_text = old_text;
		match.new_text = new_text;
		match.node_kind = node_kind;
	}

	for (int i = 0; i < matches.size() - 1; i++) {
		for (int j = i + 1; j < matches.size(); j++) {
			const MatchInfo &m1 = matches[i];
			const MatchInfo &m2 = matches[j];
			if (m1.match_start < m2.match_end && m2.match_start < m1.match_end) {
				result["error"] = "Edit #" + String::num_int64(m1.edit_index) + " and #" + String::num_int64(m2.edit_index) + " have overlapping match ranges";
				return result;
			}
		}
	}

	if (auto_indent) {
		for (int i = 0; i < matches.size(); i++) {
			MatchInfo &match = matches.write[i];

			int old_first_newline = match.old_text.find("\n");
			String old_first_line = old_first_newline == -1 ? match.old_text : match.old_text.substr(0, old_first_newline);
			String base_indent = "";
			for (int j = 0; j < old_first_line.length(); j++) {
				char32_t c = old_first_line[j];
				if (c == ' ' || c == '\t') {
					base_indent += String::chr(c);
				} else {
					break;
				}
			}

			int new_first_newline = match.new_text.find("\n");
			String new_first_line = new_first_newline == -1 ? match.new_text : match.new_text.substr(0, new_first_newline);
			String new_indent = "";
			for (int j = 0; j < new_first_line.length(); j++) {
				char32_t c = new_first_line[j];
				if (c == ' ' || c == '\t') {
					new_indent += String::chr(c);
				} else {
					break;
				}
			}

			if (!base_indent.is_empty() && new_indent.is_empty()) {
				PackedStringArray lines = match.new_text.split("\n", true);
				String indented_result = "";
				for (int line_idx = 0; line_idx < lines.size(); line_idx++) {
					String line = lines[line_idx];
					if (line.is_empty()) {
						indented_result += line;
					} else {
						indented_result += base_indent + line;
					}
					if (line_idx < lines.size() - 1) {
						indented_result += "\n";
					}
				}
				match.new_text = indented_result;
			}
		}
	}

	TypedArray<Dictionary> text_edits;
	for (int i = 0; i < matches.size(); i++) {
		const MatchInfo &match = matches[i];
		CharString old_utf8 = match.old_text.utf8();
		int start_byte = source.substr(0, match.match_start).utf8().length();
		int end_byte = start_byte + old_utf8.length();

		Dictionary text_edit;
		text_edit["start_byte"] = start_byte;
		text_edit["end_byte"] = end_byte;
		text_edit["new_text"] = match.new_text;
		text_edits.push_back(text_edit);
	}

	if (fail_on_parse_error) {
		Dictionary preview_result = apply_text_edits(file_path, text_edits, true);
		
		if (!preview_result["success"]) {
			result["error"] = preview_result["error"];
			return result;
		}
		
		if (preview_result["has_error"]) {
			result["error"] = "Edit produces parse error, rolled back";
			return result;
		}
	}

	Dictionary text_result = apply_text_edits(file_path, text_edits, dry_run);

	if (!text_result["success"]) {
		result["error"] = text_result["error"];
		return result;
	}

	result["success"] = true;
	result["new_source"] = text_result["new_source"];
	result["has_error"] = text_result["has_error"];
	result["error_count"] = text_result["error_count"];
	result["edits_applied"] = text_result["edits_applied"];

	return result;
}

String ASTManager::generate_diff(const String &old_text, const String &new_text, const String &file_name) {
	if (old_text == new_text) {
		return "";
	}

	std::string old_str = old_text.utf8().get_data();
	std::string new_str = new_text.utf8().get_data();

	std::vector<std::string> old_lines;
	std::vector<std::string> new_lines;

	std::stringstream old_stream(old_str);
	std::stringstream new_stream(new_str);
	std::string line;

	while (std::getline(old_stream, line)) {
		old_lines.push_back(line);
	}
	while (std::getline(new_stream, line)) {
		new_lines.push_back(line);
	}

	dtl::Diff<std::string, std::vector<std::string>> diff(old_lines, new_lines);
	diff.compose();

	std::stringstream result;
	result << "--- a/" << file_name.utf8().get_data() << "\n";
	result << "+++ b/" << file_name.utf8().get_data() << "\n";

	diff.composeUnifiedHunks();
	diff.printUnifiedFormat(result);

	return String::utf8(result.str().c_str());
}

Dictionary ASTManager::validate(const String &source_code) {
	Dictionary result;
	Array errors;

	CharString utf8 = source_code.utf8();
	const char *source = utf8.get_data();
	uint32_t source_len = utf8.length();
	
	TSTree *temp_tree = ts_parser_parse_string(parser, nullptr, source, source_len);

	if (!temp_tree) {
		result["valid"] = false;
		result["error_count"] = 0;
		result["errors"] = errors;
		return result;
	}

	TSNode root = ts_tree_root_node(temp_tree);
	bool has_error = ts_node_has_error(root);
	uint32_t error_count = 0;

	if (has_error) {
		std::function<void(TSNode)> collect_errors = [&](TSNode node) {
			if (ts_node_is_error(node) || ts_node_is_missing(node)) {
				Dictionary error;
				TSPoint start = ts_node_start_point(node);
				TSPoint end = ts_node_end_point(node);

				error["node_kind"] = String(ts_node_type(node));
				error["start_row"] = (int)start.row;
				error["start_col"] = (int)start.column;
				error["end_row"] = (int)end.row;
				error["end_col"] = (int)end.column;

				PackedStringArray lines = source_code.split("\n", true);
				if (start.row < (uint32_t)lines.size()) {
					error["context"] = lines[start.row];
				} else {
					error["context"] = "";
				}

				errors.push_back(error);
				error_count++;
			}

			uint32_t child_count = ts_node_child_count(node);
			for (uint32_t i = 0; i < child_count; i++) {
				collect_errors(ts_node_child(node, i));
			}
		};

		collect_errors(root);
	}

	ts_tree_delete(temp_tree);

	result["valid"] = !has_error;
	result["error_count"] = (int)error_count;
	result["errors"] = errors;

	return result;
}

void ASTManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("ping"), &ASTManager::ping);
	ClassDB::bind_method(D_METHOD("get_version"), &ASTManager::get_version);
	ClassDB::bind_method(D_METHOD("parse_test", "source_code"), &ASTManager::parse_test);
	ClassDB::bind_method(D_METHOD("open_file", "file_path", "content"), &ASTManager::open_file);
	ClassDB::bind_method(D_METHOD("close_file", "file_path"), &ASTManager::close_file);
	ClassDB::bind_method(D_METHOD("update_file", "file_path", "new_content"), &ASTManager::update_file);
	ClassDB::bind_method(D_METHOD("is_file_open", "file_path"), &ASTManager::is_file_open);
	ClassDB::bind_method(D_METHOD("get_open_files"), &ASTManager::get_open_files);
	ClassDB::bind_method(D_METHOD("get_file_source", "file_path"), &ASTManager::get_file_source);
	ClassDB::bind_method(D_METHOD("query", "file_path", "query_string"), &ASTManager::query);
	ClassDB::bind_method(D_METHOD("get_node_text", "file_path", "start_byte", "end_byte"), &ASTManager::get_node_text);
	ClassDB::bind_method(D_METHOD("get_sexp", "file_path"), &ASTManager::get_sexp);
	ClassDB::bind_method(D_METHOD("apply_text_edits", "file_path", "edits", "dry_run"), &ASTManager::apply_text_edits);
	ClassDB::bind_method(D_METHOD("apply_node_edits", "file_path", "edits", "options"), &ASTManager::apply_node_edits);
	ClassDB::bind_method(D_METHOD("generate_diff", "old_text", "new_text", "file_name"), &ASTManager::generate_diff);
	ClassDB::bind_method(D_METHOD("validate", "source_code"), &ASTManager::validate);
}
