package(
    default_visibility = ["//visibility:public"],
)

config_setting(
    name = "windows_dbg_x64",
    values = {
        "cpu": "x64_windows",
    },
)

config_setting(
    name = "windows_opt_x64",
    values = {
        "cpu": "x64_windows"
    },
)

DEFINES = [
        "NOGDI",  # Don't pollute with GDI macros in windows.h.
        "NOMINMAX",  # Don't define min/max macros in windows.h.
        "OS_WINDOWS=OS_WINDOWS",
        "PRAGMA_SUPPORTED",
        "WIN32",
        "WINVER=0x0600",
        "_CRT_SECURE_NO_DEPRECATE",
        "_WIN32",
        "_WIN32_WINNT=0x0600",
        "_WINDOWS",

        # Use math constants (M_PI, etc.) from the math library
        "_USE_MATH_DEFINES",

        # Defaults for other headers (along with OS_WINDOWS).
        "COMPILER_MSVC",

        # Doublespeak for "don't bloat namespace with incompatible winsock
        # versions that I didn't include".
        # http://msdn.microsoft.com/en-us/library/windows/desktop/ms737629.aspx
        "WIN32_LEAN_AND_MEAN=1",
    ]

# Options for everything below
all_options = [
    "-Ithirdparty\openexr",
    "-D_THREAD_SAFE",
    "/EHsc"
]

cc_binary(
    name = "openExrComposer",
    srcs = ["src/main.cc",
            "src/parser.cpp",
            "src/parser.h",
            "src/stringutils.cpp",
            "src/stringutils.h"],
    deps = ["@openexr//:ilm_imf"],
    copts = all_options,
    defines = DEFINES,
)