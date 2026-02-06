extends Node

func _ready() -> void:
	print("=== Testing ASTManager ===")
	
	var ast = ASTManager.new()
	if ast == null:
		print("ERROR: Failed to create ASTManager instance")
		return
	
	print("✓ ASTManager instance created")
	
	var ping_result = ast.ping()
	print("ping() returned: ", ping_result)
	if ping_result == "pong":
		print("✓ ping() test passed")
	else:
		print("ERROR: ping() test failed")
	
	var version = ast.get_version()
	print("get_version() returned: ", version)
	if version == "0.1.0":
		print("✓ get_version() test passed")
	else:
		print("ERROR: get_version() test failed")
	
	print("=== All tests completed ===")
