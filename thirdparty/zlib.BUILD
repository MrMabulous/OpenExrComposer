package(
    default_visibility = ["//visibility:public"],
    features = [
        "-layering_check",
        "-parse_headers",
    ],
)

licenses(["notice"])  # BSD/MIT-like license (for zlib)

exports_files([
    "LICENSE",
])

config_setting(
    name = "windows_x86_64",
    values = {"cpu": "x64_windows"},
    visibility = ["//visibility:public"],
)

cc_library(
    name = "zlib",
    srcs = [
        "adler32.c",
        "cpu_features.c",
        "compress.c",
        "crc32.c",
        "deflate.c",
        "gzclose.c",
        "gzlib.c",
        "gzread.c",
        "gzwrite.c",
        "infback.c",
        "inffast.c",
        "inflate.c",
        "inftrees.c",
        "trees.c",
        "uncompr.c",
        "zutil.c",
    ],
    hdrs = [
        "contrib/optimizations/insert_string.h",
        "cpu_features.h",
        "crc32.h",
        "deflate.h",
        "gzguts.h",
        "inffast.h",
        "inffixed.h",
        "inflate.h",
        "inftrees.h",
        "trees.h",
        "zconf.h",
        "zlib.h",
        "zutil.h",
    ],
    defines = [] + select({
        ":windows_x86_64": [
            "X86_WINDOWS",
        ],
    }),
    visibility = ["//visibility:public"],
)