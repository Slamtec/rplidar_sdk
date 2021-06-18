load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "rplidar_driver",
    srcs = [
        "src/rplidar_driver.cpp",
        "src/hal/thread.cpp",
    ] + select({
        "@bazel_tools//src/conditions:windows": glob(["src/arch/win32/*.cpp"]),
        "//conditions:default": glob(["src/arch/linux/*.cpp"]),
    }),
    hdrs = glob(["include/*.h"]) + select({
        "@bazel_tools//src/conditions:windows": glob(["src/arch/win32/*.hpp"]) + glob(["src/arch/win32/*.h"]),
        "//conditions:default": glob(["src/arch/linux/*.hpp"]) + glob(["src/arch/linux/*.h"]),
    }) + glob([
        "src/*.h",
        "src/hal/*.h",
    ]),
    # NOTICE: Ugly hack is ugly, but required in builds that enable -werr
    # NOTICE: mainly because there is magic in this lib I don't want to attempt to fix.
    copts = [
        "-Wno-unused-variable",
        "-Wno-sign-compare",
        "-Wno-unused-but-set-variable",
        "-Wno-parentheses",
        "-Wno-unused-result",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
)
