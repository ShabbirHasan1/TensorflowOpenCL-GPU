package(default_visibility = ["//visibility:public"])

load(
    "//tensorflow:tensorflow.bzl",
    "tf_copts",
    "if_android_arm64",
)

cc_binary(
    name = "opencl-monte-carlo-bench",
    srcs = [
        "Monte-Carlo/monteCarloEngine.c",
    ],
    copts = [
        "-ffunction-sections",
        "-fdata-sections",
        "-fPIE",
        "-pie",
        "-fexceptions",
    ],
    linkopts = [
        "-fPIE",
        "-pie",
        "-landroid",
        "-latomic",
        "-ldl",
        "-llog",
        "-lm",
        "-z defs",
        "-s",
        "-Wl,--gc-sections",
        "-fuse-ld=gold",
    ],
    includes = [
        "Monte-Carlo/"
    ],
    linkstatic = 1,
    tags = [
        "manual",
        "notap",
    ],
    deps = [
        "//external:android_opencl_libs",
    ],
)
