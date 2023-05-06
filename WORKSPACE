load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

http_archive(
    name = "catch",
    url = "https://github.com/cgrinker/Catch2/archive/5e6488fd9949cb41d717a72c8c4603b7e37d68cd.zip",
    sha256 = "91e3e0610572adefa301a6e55ac48ab0a3c8ff61787ce6930e346ff36e86905c",
    strip_prefix = "Catch2-5e6488fd9949cb41d717a72c8c4603b7e37d68cd",
)


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
    name = "com_github_nelhage_rules_boost",
    url = "https://github.com/nelhage/rules_boost/archive/9c1e42a7d1fa1a9778aa91e5450f6d204936fe8b.tar.gz",
    strip_prefix = "rules_boost-9c1e42a7d1fa1a9778aa91e5450f6d204936fe8b",
    sha256 = "ff627f3013adb8096d845ef0cfd12a9327fa48d90db501671d5c756b9d1b990b",
)
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()


git_repository(
    name = "benchmark",
    remote = "git@github.com:google/benchmark.git",
    commit = "d572f4777349d43653b21d6c2fc63020ab326db2",
    shallow_since = "1668175263 +0000"
)
git_repository(
    name = "flecs",
    remote = "git@github.com:SanderMertens/flecs.git",
    commit = "0fab02f47f330b863a67a8e253ceba4956861ef6",
    shallow_since = "1681676287 -0700"
)
new_git_repository(
    name = "pico",
    remote = "git@github.com:empyreanx/pico_headers.git",
    commit = "20e19aa5447d016bbebc25dba5f68b48d56b8ef2",
    shallow_since = "1682712996 -0400",
    build_file_content = """
cc_library(
    name = "ecs",
    visibility = ["//visibility:public"],
    hdrs = ["pico_ecs.h"],
)
    """,
)
new_git_repository(
    name = "entt",
    remote = "git@github.com:skypjack/entt.git",
    commit = "fef921132cae7588213d0f9bcd2fb9c8ffd8b7fc",
    shallow_since = "1669969760 +0100",
    build_file_content = """
cc_library(
    name = "entt",
    visibility = ["//visibility:public"],
    hdrs = glob(["src/**/*.h", "src/**/*.hpp"]),
    strip_include_prefix = "src",
)
    """,
)