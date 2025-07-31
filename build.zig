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

    const server = b.addExecutable(.{
        .name = "w1re-server",
        .optimize = optimize,
        .target = target,
    });

    server.linkLibC();
    server.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = &.{"--std=c23"},
        .files = &.{"server/main.c"},
    });

    const client = b.addExecutable(.{
        .name = "w1re-client",
        .optimize = optimize,
        .target = target,
    });

    client.linkLibC();
    client.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = &.{"--std=c23"},
        .files = &.{"client/main.c"},
    });

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
