#!/usr/bin/env python

env = SConscript("thirdparty/godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
env.Append(CPPPATH=["thirdparty/tree-sitter/lib/include"])
env.Append(CPPPATH=["thirdparty/tree-sitter-gdscript/src"])

sources = Glob("src/*.cpp")
sources += [
    "thirdparty/tree-sitter/lib/src/lib.c",
    "thirdparty/tree-sitter-gdscript/src/parser.c",
    "thirdparty/tree-sitter-gdscript/src/scanner.c"
]

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/ai_script_plugin/bin/libast.{}.{}.framework/libast.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/ai_script_plugin/bin/libast.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/ai_script_plugin/bin/libast.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "addons/ai_script_plugin/bin/libast{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

env.NoCache(library)
Default(library)
