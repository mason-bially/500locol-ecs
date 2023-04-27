load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "catch",
    url = "https://github.com/cgrinker/Catch2/archive/5e6488fd9949cb41d717a72c8c4603b7e37d68cd.zip",
    sha256 = "91e3e0610572adefa301a6e55ac48ab0a3c8ff61787ce6930e346ff36e86905c",
    strip_prefix = "Catch2-5e6488fd9949cb41d717a72c8c4603b7e37d68cd",
)
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
