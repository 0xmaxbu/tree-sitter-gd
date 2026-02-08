@tool
extends EditorPlugin

var _ast: ASTManager
var _pass_count := 0
var _fail_count := 0
var _section_name := ""

# ──────────────────────────────────────────────
# 测试辅助
# ──────────────────────────────────────────────

func _log(msg: String) -> void:
	print("[AST Test] ", msg)

func _begin_section(name: String) -> void:
	_section_name = name
	_log("────── %s ──────" % name)

func _check(condition: bool, label: String) -> bool:
	if condition:
		_pass_count += 1
		_log("  ✓ %s" % label)
		return true
	else:
		_fail_count += 1
		_log("  ✗ FAIL: %s" % label)
		return false

func _check_eq(actual: Variant, expected: Variant, label: String) -> bool:
	if actual == expected:
		return _check(true, label)
	else:
		_fail_count += 1
		_log("  ✗ FAIL: %s (expected=%s, got=%s)" % [label, str(expected), str(actual)])
		return false

func _check_contains(text: String, substr: String, label: String) -> bool:
	return _check(substr in text, "%s (looking for '%s')" % [label, substr])

# ──────────────────────────────────────────────
# 生命周期
# ──────────────────────────────────────────────

func _enter_tree() -> void:
	_log("═══════════════════════════════════════════")
	_log("ASTManager GDExtension — 完整测试开始")
	_log("═══════════════════════════════════════════")

	_ast = ASTManager.new()

	_test_section_0_ping()
	_test_section_1_file_lifecycle()
	_test_section_2_parse_errors()
	_test_section_3_sexp_discovery()
	_test_section_4_query()
	_test_section_5_get_node_text()
	_test_section_6_apply_text_edits()
	_test_section_7_apply_node_edits()
	_test_section_8_auto_indent()
	_test_section_9_generate_diff()
	_test_section_10_validate()

	_log("")
	_log("═══════════════════════════════════════════")
	_log("结果: %d 通过, %d 失败, 共 %d 项" % [_pass_count, _fail_count, _pass_count + _fail_count])
	if _fail_count == 0:
		_log("★ ALL TESTS PASSED ★")
	else:
		_log("✗ 有 %d 项测试失败，请检查上方输出" % _fail_count)
	_log("═══════════════════════════════════════════")


func _exit_tree() -> void:
	if _ast:
		# 关闭所有可能残留的缓存
		for f in _ast.get_open_files():
			_ast.close_file(f)
		_ast = null
	_log("插件已卸载")


# ──────────────────────────────────────────────
# Section 0: 基本连通性
# ──────────────────────────────────────────────

func _test_section_0_ping() -> void:
	_begin_section("0. 基本连通性")

	_check(_ast != null, "ASTManager 实例化成功")
	_check_eq(_ast.ping(), "pong", "ping() == 'pong'")

	var ver := _ast.get_version()
	_check(ver.length() > 0, "get_version() 非空: '%s'" % ver)


# ──────────────────────────────────────────────
# Section 1: 文件生命周期
# ──────────────────────────────────────────────

func _test_section_1_file_lifecycle() -> void:
	_begin_section("1. 文件生命周期")

	var code := "extends Node\n\nfunc _ready() -> void:\n\tpass\n"

	# open_file
	var r := _ast.open_file("test://lifecycle", code)
	_check(r["success"] == true, "open_file 成功")
	_check(r["has_error"] == false, "无解析错误")
	_check(r["node_count"] > 0, "节点数 > 0: %d" % r["node_count"])
	_check_eq(r["file_path"], "test://lifecycle", "file_path 回显")

	# is_file_open
	_check_eq(_ast.is_file_open("test://lifecycle"), true, "is_file_open == true")
	_check_eq(_ast.is_file_open("test://not_exist"), false, "is_file_open 不存在 == false")

	# get_open_files
	var files := _ast.get_open_files()
	_check("test://lifecycle" in files, "get_open_files 包含 test://lifecycle")

	# get_file_source
	var src := _ast.get_file_source("test://lifecycle")
	_check_eq(src, code, "get_file_source 内容一致")
	_check_eq(_ast.get_file_source("test://not_exist"), "", "不存在的文件返回空字符串")

	# update_file
	var new_code := "extends Node2D\n\nfunc hello():\n\tprint(\"hi\")\n"
	var u := _ast.update_file("test://lifecycle", new_code)
	_check(u["success"] == true, "update_file 成功")
	_check_eq(_ast.get_file_source("test://lifecycle"), new_code, "update 后内容已替换")

	# update_file 对未打开的文件
	var u2 := _ast.update_file("test://not_open", "x")
	_check(u2["success"] == false, "update 未打开的文件失败")

	# close_file
	_check_eq(_ast.close_file("test://lifecycle"), true, "close_file 成功")
	_check_eq(_ast.is_file_open("test://lifecycle"), false, "close 后 is_file_open == false")
	_check_eq(_ast.close_file("test://lifecycle"), false, "重复 close 返回 false")


# ──────────────────────────────────────────────
# Section 2: 解析错误检测
# ──────────────────────────────────────────────

func _test_section_2_parse_errors() -> void:
	_begin_section("2. 解析错误检测")

	# 无错误的代码
	var good := "extends Node\n\nvar x: int = 10\n\nfunc foo() -> void:\n\tpass\n"
	var r1 := _ast.open_file("test://good", good)
	_check_eq(r1["has_error"], false, "正确代码 has_error == false")
	_check_eq(r1["error_count"], 0, "正确代码 error_count == 0")
	_ast.close_file("test://good")

	# 有错误的代码
	var bad := "extends Node\n\nfunc broken(\n\tvar x =\n"
	var r2 := _ast.open_file("test://bad", bad)
	_check_eq(r2["success"], true, "有错误的代码也能解析 (tree-sitter 容错)")
	_check_eq(r2["has_error"], true, "has_error == true")
	_check(r2["error_count"] > 0, "error_count > 0: %d" % r2["error_count"])

	# error_ranges 格式
	if r2["error_ranges"].size() > 0:
		var err: Dictionary = r2["error_ranges"][0]
		_check(err.has("start_row"), "error_range 有 start_row")
		_check(err.has("start_col"), "error_range 有 start_col")
		_check(err.has("end_row"), "error_range 有 end_row")
		_check(err.has("end_col"), "error_range 有 end_col")
		_check(err.has("start_byte"), "error_range 有 start_byte")
		_check(err.has("end_byte"), "error_range 有 end_byte")
		_log("    首个错误位置: 行 %d, 列 %d" % [
			err.get("start_row", -1) + 1,
			err.get("start_col", -1) + 1,
		])
		_log("    error_range 实际字段: %s" % str(err.keys()))
	else:
		_check(false, "error_ranges 应非空")

	_ast.close_file("test://bad")


# ──────────────────────────────────────────────
# Section 3: S-expression 与节点类型发现
# ──────────────────────────────────────────────

func _test_section_3_sexp_discovery() -> void:
	_begin_section("3. S-expression 与节点类型发现")

	var code := """extends CharacterBody2D

signal health_changed(new_value: int)

@export var max_health: int = 100
@export var speed: float = 300.0

var _health: int = 100

func _ready() -> void:
	_health = max_health

func take_damage(amount: int) -> void:
	_health -= amount
	if _health <= 0:
		die()

func die() -> void:
	queue_free()
"""

	_ast.open_file("test://sexp", code)

	var sexp := _ast.get_sexp("test://sexp")
	_check(sexp.length() > 0, "get_sexp 返回非空, 长度=%d" % sexp.length())

	# 打印完整 sexp 供人工审阅（缩短显示）
	_log("  ── 完整 S-expression（前 2000 字符）──")
	var lines := sexp.substr(0, 2000).split("\n")
	for line in lines:
		_log("    " + line)
	if sexp.length() > 2000:
		_log("    ... (截断, 总长 %d)" % sexp.length())
	_log("  ── S-expression 结束 ──")

	# 检测根节点类型
	var root_type := ""
	if sexp.begins_with("("):
		var space_pos := sexp.find(" ")
		if space_pos > 1:
			root_type = sexp.substr(1, space_pos - 1)
	_check(root_type.length() > 0, "根节点类型: '%s'" % root_type)

	# 打印发现的节点类型（辅助后续 query 编写）
	_log("  ── 请从 sexp 中辨认以下节点类型名 ──")
	_log("    函数定义节点:    (在 sexp 中搜索包含 _ready 的括号结构)")
	_log("    变量声明节点:    (在 sexp 中搜索包含 _health 的括号结构)")
	_log("    signal 声明节点: (在 sexp 中搜索包含 health_changed 的括号结构)")
	_log("    export 注解节点: (在 sexp 中搜索包含 max_health 的括号结构)")
	_log("    extends 声明:    (在 sexp 中搜索包含 CharacterBody2D 的括号结构)")

	# 尝试用未打开的文件
	_check_eq(_ast.get_sexp("test://not_exist"), "", "未打开的文件 get_sexp 返回空")

	_ast.close_file("test://sexp")


# ──────────────────────────────────────────────
# Section 4: Query
# ──────────────────────────────────────────────

func _test_section_4_query() -> void:
	_begin_section("4. Query 查询")

	var code := "extends Node\n\nvar health: int = 100\nvar speed: float = 200.0\n\nfunc _ready() -> void:\n\tpass\n\nfunc take_damage(amount: int) -> void:\n\thealth -= amount\n"
	_ast.open_file("test://query", code)

	# 尝试查询函数定义（节点类型名可能因 grammar 不同）
	var query_patterns := [
		'(function_definition name: (name) @fn_name)',
		'(function_definition name: (identifier) @fn_name)',
		'(function_def name: (name) @fn_name)',
	]

	var working_pattern := ""
	var fn_names: Array = []

	for pattern in query_patterns:
		var r := _ast.query("test://query", pattern)
		if r["success"] and r["matches"].size() > 0:
			working_pattern = pattern
			for m in r["matches"]:
				for c in m["captures"]:
					if c["name"] == "fn_name":
						fn_names.append(c["text"])
			break

	if working_pattern != "":
		_check(true, "找到可用的函数查询 pattern: '%s'" % working_pattern)
		_check("_ready" in fn_names, "query 找到 _ready")
		_check("take_damage" in fn_names, "query 找到 take_damage")
		_log("    匹配的函数名: %s" % str(fn_names))

		# 验证 capture 字段完整性
		var r := _ast.query("test://query", working_pattern)
		var first_capture: Dictionary = r["matches"][0]["captures"][0]
		_check(first_capture.has("name"), "capture 有 name 字段")
		_check(first_capture.has("node_kind"), "capture 有 node_kind 字段")
		_check(first_capture.has("text"), "capture 有 text 字段")
		_check(first_capture.has("start_byte"), "capture 有 start_byte")
		_check(first_capture.has("end_byte"), "capture 有 end_byte")
		_check(first_capture.has("start_row"), "capture 有 start_row (0-based)")
		_check(first_capture.has("start_col"), "capture 有 start_col (0-based)")
		_check(first_capture.has("end_row"), "capture 有 end_row")
		_check(first_capture.has("end_col"), "capture 有 end_col")
		_log("    首个 capture: name=%s, kind=%s, text=%s, row=%d" % [
			first_capture["name"],
			first_capture["node_kind"],
			first_capture["text"],
			first_capture["start_row"],
		])
	else:
		_check(false, "所有尝试的函数查询 pattern 均失败，请查看 Section 3 的 sexp 确认节点类型名")

	# 错误的 query 语法
	var bad_q := _ast.query("test://query", "(((broken syntax")
	_check_eq(bad_q["success"], false, "非法 query 返回 success=false")
	_check(bad_q["error"].length() > 0, "非法 query 有错误信息: '%s'" % bad_q["error"].substr(0, 80))

	# 未打开的文件
	var no_file := _ast.query("test://not_open", '(name) @n')
	_check_eq(no_file["success"], false, "未打开文件的 query 返回 success=false")

	_ast.close_file("test://query")


# ──────────────────────────────────────────────
# Section 5: get_node_text
# ──────────────────────────────────────────────

func _test_section_5_get_node_text() -> void:
	_begin_section("5. get_node_text")

	var code := "extends Node\nvar x = 42\n"
	_ast.open_file("test://nodetext", code)

	var code_bytes := code.to_utf8_buffer()
	# "extends" 在 code 的 0..7 字节
	var text := _ast.get_node_text("test://nodetext", 0, 7)
	_check_eq(text, "extends", "get_node_text(0, 7) == 'extends'")

	# "42" 的位置
	var pos_42 := code.find("42")
	var byte_start := code.substr(0, pos_42).to_utf8_buffer().size()
	var byte_end := byte_start + "42".to_utf8_buffer().size()
	var text_42 := _ast.get_node_text("test://nodetext", byte_start, byte_end)
	_check_eq(text_42, "42", "get_node_text 正确取出 '42'")

	# 未打开的文件
	_check_eq(_ast.get_node_text("test://nope", 0, 5), "", "未打开文件返回空")

	_ast.close_file("test://nodetext")


# ──────────────────────────────────────────────
# Section 6: apply_text_edits (字节级编辑)
# ──────────────────────────────────────────────

func _test_section_6_apply_text_edits() -> void:
	_begin_section("6. apply_text_edits (字节级编辑)")

	var code := "extends Node\n\nvar health: int = 100\n"
	_ast.open_file("test://textedit", code)

	# 找到 "100" 的字节位置
	var pos := code.find("100")
	var start := code.substr(0, pos).to_utf8_buffer().size()
	var end := start + "100".to_utf8_buffer().size()

	# 替换 100 → 999
	var r := _ast.apply_text_edits("test://textedit", [
		{"start_byte": start, "end_byte": end, "new_text": "999"}
	], false)

	_check_eq(r["success"], true, "apply_text_edits 成功")
	_check_contains(r["new_source"], "999", "新源码包含 999")
	_check(not ("100" in r["new_source"]), "新源码不包含 100")
	_check_eq(r["edits_applied"], 1, "edits_applied == 1")
	# 缓存已更新
	_check_contains(_ast.get_file_source("test://textedit"), "999", "缓存已更新为 999")

	# dry_run 测试
	_ast.update_file("test://textedit", code)  # 重置
	var pos2 := code.find("100")
	var start2 := code.substr(0, pos2).to_utf8_buffer().size()
	var end2 := start2 + "100".to_utf8_buffer().size()

	var r2 := _ast.apply_text_edits("test://textedit", [
		{"start_byte": start2, "end_byte": end2, "new_text": "0"}
	], true)

	_check_eq(r2["success"], true, "dry_run 成功")
	_check_contains(r2["new_source"], "0", "dry_run 预览包含 0")
	_check_contains(_ast.get_file_source("test://textedit"), "100", "dry_run 后缓存未变（仍为 100）")

	_ast.close_file("test://textedit")


# ──────────────────────────────────────────────
# Section 7: apply_node_edits (文本匹配编辑)
# ──────────────────────────────────────────────

func _test_section_7_apply_node_edits() -> void:
	_begin_section("7. apply_node_edits (文本匹配编辑)")

	var code := "extends Node\n\nvar health: int = 100\nvar speed: float = 200.0\n\nfunc _ready() -> void:\n\thealth = 100\n\tprint(\"ready\")\n\nfunc die() -> void:\n\tqueue_free()\n"
	_ast.open_file("test://nodeedit", code)

	# 7.1 单处替换
	var r1 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "var health: int = 100", "new_text": "var health: int = 999"}
	], {})
	_check_eq(r1["success"], true, "7.1 单处替换成功")
	_check_contains(r1["new_source"], "999", "7.1 新源码含 999")
	_check_eq(r1["edits_applied"], 1, "7.1 edits_applied == 1")

	# 7.2 多处替换（重置后）
	_ast.update_file("test://nodeedit", code)
	var r2 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "var health: int = 100", "new_text": "var hp: int = 100"},
		{"old_text": "var speed: float = 200.0", "new_text": "var spd: float = 300.0"},
	], {})
	_check_eq(r2["success"], true, "7.2 多处替换成功")
	_check_eq(r2["edits_applied"], 2, "7.2 edits_applied == 2")
	_check_contains(r2["new_source"], "hp", "7.2 包含 hp")
	_check_contains(r2["new_source"], "spd", "7.2 包含 spd")

	# 7.3 old_text 不存在
	_ast.update_file("test://nodeedit", code)
	var r3 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "this_does_not_exist_anywhere", "new_text": "x"}
	], {})
	_check_eq(r3["success"], false, "7.3 匹配失败")
	_check(r3["error"].length() > 0, "7.3 有错误信息")
	_log("    错误信息: %s" % r3["error"])
	# 缓存应未变
	_check_eq(_ast.get_file_source("test://nodeedit"), code, "7.3 匹配失败后缓存未变")

	# 7.4 dry_run
	_ast.update_file("test://nodeedit", code)
	var r4 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "var health: int = 100", "new_text": "var hp: int = 0"}
	], {"dry_run": true})
	_check_eq(r4["success"], true, "7.4 dry_run 成功")
	_check_contains(r4["new_source"], "hp", "7.4 预览包含 hp")
	_check_eq(_ast.get_file_source("test://nodeedit"), code, "7.4 dry_run 缓存未变")

	# 7.5 删除（new_text 为空）
	_ast.update_file("test://nodeedit", code)
	var r5 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "\tprint(\"ready\")\n", "new_text": ""}
	], {})
	_check_eq(r5["success"], true, "7.5 删除成功")
	_check(not ("print(\"ready\")" in r5["new_source"]), "7.5 print(ready) 已删除")

	# 7.6 编辑后语法检查
	_ast.update_file("test://nodeedit", code)
	var r6 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "var health: int = 100", "new_text": "var health: int = 999"}
	], {})
	_check(r6.has("has_error"), "7.6 返回含 has_error 字段")
	_check_eq(r6["has_error"], false, "7.6 合法编辑无语法错误")

	# 7.7 部分匹配失败 → 全部回滚
	_ast.update_file("test://nodeedit", code)
	var r7 := _ast.apply_node_edits("test://nodeedit", [
		{"old_text": "var health: int = 100", "new_text": "var x: int = 0"},
		{"old_text": "NOT_EXIST", "new_text": "y"},
	], {})
	_check_eq(r7["success"], false, "7.7 部分失败 → 整体失败")
	_check_eq(_ast.get_file_source("test://nodeedit"), code, "7.7 原子性保证: 缓存未变")

	_ast.close_file("test://nodeedit")


# ──────────────────────────────────────────────
# Section 8: 自动缩进
# ──────────────────────────────────────────────

func _test_section_8_auto_indent() -> void:
	_begin_section("8. 自动缩进")

	var code := "extends Node\n\nfunc _ready() -> void:\n\thealth = 100\n\tprint(\"ready\")\n"
	_ast.open_file("test://indent", code)

	# 8.1 auto_indent 开启：new_text 零缩进 → 自动补齐
	var r1 := _ast.apply_node_edits("test://indent", [
		{
			"old_text": "\thealth = 100\n\tprint(\"ready\")",
			"new_text": "health = 999\nprint(\"updated\")\nprint(\"done\")",
		}
	], {"auto_indent": true})
	_check_eq(r1["success"], true, "8.1 auto_indent 编辑成功")

	# 验证每行缩进
	var all_indented := true
	for line in r1["new_source"].split("\n"):
		if "999" in line and not line.begins_with("\t"):
			all_indented = false
			_log("    缩进缺失: '%s'" % line)
		if "updated" in line and not line.begins_with("\t"):
			all_indented = false
			_log("    缩进缺失: '%s'" % line)
		if "done" in line and not line.begins_with("\t"):
			all_indented = false
			_log("    缩进缺失: '%s'" % line)
	_check(all_indented, "8.1 所有新增行均有 \\t 缩进")

	# 8.2 auto_indent 关闭：原样替换
	_ast.update_file("test://indent", code)
	var r2 := _ast.apply_node_edits("test://indent", [
		{
			"old_text": "\thealth = 100\n\tprint(\"ready\")",
			"new_text": "no_indent_line1\nno_indent_line2",
		}
	], {"auto_indent": false})
	_check_eq(r2["success"], true, "8.2 auto_indent=false 成功")
	_check_contains(r2["new_source"], "no_indent_line1", "8.2 原样替换")
	# 验证没有添加缩进
	for line in r2["new_source"].split("\n"):
		if "no_indent_line1" in line:
			_check(not line.begins_with("\t"), "8.2 auto_indent=false 不加缩进: '%s'" % line)
			break

	# 8.3 new_text 已有缩进 → auto_indent 不画蛇添足
	_ast.update_file("test://indent", code)
	var r3 := _ast.apply_node_edits("test://indent", [
		{
			"old_text": "\thealth = 100\n\tprint(\"ready\")",
			"new_text": "\thealth = 999\n\tprint(\"already indented\")",
		}
	], {"auto_indent": true})
	_check_eq(r3["success"], true, "8.3 已缩进 new_text 不重复缩进")
	# 验证没有双重缩进
	var no_double := true
	for line in r3["new_source"].split("\n"):
		if "already indented" in line:
			if line.begins_with("\t\t"):
				no_double = false
				_log("    双重缩进: '%s'" % line)
	_check(no_double, "8.3 无双重缩进")

	_ast.close_file("test://indent")


# ──────────────────────────────────────────────
# Section 9: generate_diff
# ──────────────────────────────────────────────

func _test_section_9_generate_diff() -> void:
	_begin_section("9. generate_diff")

	var old_text := "extends Node\n\nvar health: int = 100\n\nfunc _ready():\n\tpass\n"
	var new_text := "extends Node\n\nvar health: int = 999\n\nfunc _ready():\n\tprint(\"hi\")\n"

	var diff := _ast.generate_diff(old_text, new_text, "test.gd")
	_check(diff.length() > 0, "diff 非空")
	_check_contains(diff, "---", "包含 --- 头")
	_check_contains(diff, "+++", "包含 +++ 头")
	_check_contains(diff, "@@", "包含 @@ 块头")
	_log("  ── diff 输出 ──")
	for line in diff.split("\n"):
		_log("    " + line)

	# 相同内容
	var same_diff := _ast.generate_diff(old_text, old_text, "same.gd")
	_check(same_diff.length() == 0 or same_diff.strip_edges().length() == 0,
		"相同内容 diff 为空, 实际长度=%d" % same_diff.length())


# ──────────────────────────────────────────────
# Section 10: validate
# ──────────────────────────────────────────────

func _test_section_10_validate() -> void:
	_begin_section("10. validate")

	# 合法代码
	var v1 := _ast.validate("extends Node\n\nfunc _ready():\n\tpass\n")
	_check_eq(v1["valid"], true, "合法代码 valid == true")
	_check_eq(v1["error_count"], 0, "合法代码 error_count == 0")
	_check_eq(v1["errors"].size(), 0, "合法代码 errors 为空数组")

	# 非法代码
	var v2 := _ast.validate("func broken(\n\tvar x =\n")
	_check_eq(v2["valid"], false, "非法代码 valid == false")
	_check(v2["error_count"] > 0, "非法代码 error_count > 0: %d" % v2["error_count"])
	_check(v2["errors"].size() > 0, "非法代码 errors 非空")

	if v2["errors"].size() > 0:
		var err: Dictionary = v2["errors"][0]
		_check(err.has("start_row"), "error 有 start_row")
		_check(err.has("start_col"), "error 有 start_col")
		_check(err.has("end_row"), "error 有 end_row")
		_check(err.has("end_col"), "error 有 end_col")
		_check(err.has("context"), "error 有 context")
		_check(err.has("parent_kind"), "error 有 parent_kind")
		_log("    error[0]: 行=%d, context='%s', parent='%s'" % [
			err.get("start_row", -1) + 1,
			str(err.get("context", "N/A")).strip_edges().substr(0, 60),
			err.get("parent_kind", "N/A"),
		])
		# 打印实际返回的所有 key，方便排查
		_log("    error[0] 实际字段: %s" % str(err.keys()))

	# 空代码
	var v3 := _ast.validate("")
	_check_eq(v3["valid"], true, "空代码 valid == true (无 ERROR 节点)")

	# 只有 extends
	var v4 := _ast.validate("extends Node\n")
	_check_eq(v4["valid"], true, "仅 extends 语句 valid == true")

	# validate 不影响缓存
	_check_eq(_ast.get_open_files().size(), 0, "validate 不会打开任何文件缓存")
