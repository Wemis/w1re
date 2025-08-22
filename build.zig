const std = @import("std");

const RunMode = enum {
    Server,
    Client,
    ServerAndClient,
};

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const build_component = b.option(RunMode, "component", "Choose to build server or client or both") orelse .ServerAndClient;
    const enable_sanitizers = b.option(bool, "sanitizers", "pass true if you need to enable sanitizers for your code") orelse true;

    const server = b.addExecutable(.{
        .name = "w1re-server",
        .optimize = optimize,
        .target = target,
    });

    const shared_files: []const []const u8 = &.{
        "shared/slice.c",
        "shared/hex.c",
        "shared/serializer.c",
        "client/core/account.c",
    };

    const shared_flags: []const []const u8 = if (enable_sanitizers) &.{
        "-fsanitize=address",
        "-fsanitize=undefined",
        "-fsanitize=leak",
        "-fsanitize-address-use-after-return=always",
        "-fno-omit-frame-pointer",
        "-fno-optimize-sibling-calls",
        "--std=c23",
    } else &.{"--std=c23"};

    server.linkLibC();
    server.addCSourceFiles(.{ .files = &.{ "libs/cjson/cJSON.c", "libs/base58/base58.c" } });
    server.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = shared_flags,
        .files = @as([]const []const u8, &.{ "server/main.c", "server/commands.c" }) ++ shared_files,
    });
    server.addIncludePath(b.path("libs"));
    if (enable_sanitizers) server.linkSystemLibrary("asan");
    server.linkSystemLibrary("sodium");

    const client = b.addExecutable(.{
        .name = "w1re-client",
        .optimize = optimize,
        .target = target,
    });

    client.linkLibC();
    client.addCSourceFiles(.{ .files = &.{ "libs/base58/base58.c", "libs/cjson/cJSON.c" } });
    client.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = shared_flags,
        .files = @as([]const []const u8, &.{ "client/main.c", "client/core/message.c", "client/network.c" }) ++ shared_files,
    });
    client.addIncludePath(b.path("libs"));
    if (enable_sanitizers) client.linkSystemLibrary("asan");
    client.addLibraryPath(b.path("libs/SDL3/lib/"));
    client.addLibraryPath(b.path("libs/SDL3_ttf/lib"));
    client.addLibraryPath(b.path("libs/SDL3_image/lib"));
    client.linkSystemLibrary("sodium");
    client.linkSystemLibrary("event");
    client.linkSystemLibrary("freetype");
    client.linkSystemLibrary("harfbuzz");
    client.linkSystemLibrary("SDL3");
    client.linkSystemLibrary("SDL3_ttf");
    client.linkSystemLibrary("SDL3_image");

    switch (build_component) {
        .Client => b.installArtifact(client),
        .Server => b.installArtifact(server),
        .ServerAndClient => {
            b.installArtifact(client);
            b.installArtifact(server);
        },
    }

    const run_artifacts = switch (build_component) {
        .Client => &.{b.addRunArtifact(client)},
        .Server => &.{b.addRunArtifact(server)},
        .ServerAndClient => &.{
            b.addRunArtifact(server),
            b.addRunArtifact(client),
        },
    };

    const run_step = b.step("run", "Run selected components");
    for (run_artifacts) |artifact| {
        run_step.dependOn(&artifact.step);
    }
}
