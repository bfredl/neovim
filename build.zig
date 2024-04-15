const std = @import("std");

pub fn build(b: *std.Build) void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard optimization options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
    // set a preferred release mode, allowing the user to decide how to optimize.
    const optimize = b.standardOptimizeOption(.{});

    const embedded_data = b.addModule("embedded_data", .{
        .root_source_file = .{ .path = "runtime/embedded_data.zig" },
    });

    const ziglua = b.dependency("ziglua", .{
        .target = target,
        .optimize = optimize,
        .lang = .lua51,
        .shared = false,
    });

    const nlua0_exe = b.addExecutable(.{
        .name = "nlua0",
        .root_source_file = .{ .path = "src/nlua0.zig" },
        .target = target,
        .optimize = optimize,
    });
    const nlua0_mod = &nlua0_exe.root_module;

    // TODO: need mod.addLibraryPathFromDependency(ziglua)
    // nlua0_mod.include_dirs.append(nlua0_mod.owner.allocator, .{ .other_step = ziglua_mod }) catch @panic("OOM");

    const nlua_modules_sources = [_][]const u8{
        "src/mpack/lmpack.c",
        "src/mpack/mpack_core.c",
        "src/mpack/object.c",
        "src/mpack/conv.c",
        "src/mpack/rpc.c",
    };

    const flags = [_][]const u8{
        // Standard version used in Lua Makefile
        "-std=gnu99",
    };

    nlua0_mod.addImport("ziglua", ziglua.module("ziglua"));
    nlua0_mod.addImport("embedded_data", embedded_data);
    // addImport already links by itself. but we need headers as well..
    nlua0_mod.linkLibrary(ziglua.artifact("lua"));

    nlua0_mod.addIncludePath(b.path("src"));
    nlua0_mod.addIncludePath(b.path("src/includes_fixmelater"));
    nlua0_mod.addCSourceFiles(.{
        .files = &nlua_modules_sources,
        .flags = &flags,
    });

    // This declares intent for the executable to be installed into the
    // standard location when the user invokes the "install" step (the default
    // step when running `zig build`).
    // TODO: this should be deleted as nlua0 is only used as a build tool!
    b.installArtifact(nlua0_exe);

    // This *creates* a Run step in the build graph, to be executed when another
    // step is evaluated that depends on it. The next line below will establish
    // such a dependency.
    const run_cmd = b.addRunArtifact(nlua0_exe);

    // By making the run step depend on the install step, it will be run from the
    // installation directory rather than directly from within the cache directory.
    // This is not necessary, however, if the application depends on other installed
    // files, this ensures they will be present and in the expected location.
    run_cmd.step.dependOn(b.getInstallStep());

    // like this: `zig build nlua0 -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("nlua0", "Run nlua0 build tool");
    run_step.dependOn(&run_cmd.step);

    const exe_unit_tests = b.addTest(.{
        .root_source_file = .{ .path = "src/nlua0.zig" },
        .target = target,
        .optimize = optimize,
    });

    const test_mod = &exe_unit_tests.root_module;
    // TODO: figure out how these should be shared :P
    test_mod.addImport("ziglua", ziglua.module("ziglua"));
    test_mod.addImport("embedded_data", embedded_data);
    // addImport already links by itself. but we need headers as well..
    test_mod.linkLibrary(ziglua.artifact("lua"));

    test_mod.addIncludePath(b.path("src"));
    test_mod.addIncludePath(b.path("src/includes_fixmelater"));
    test_mod.addCSourceFiles(.{
        .files = &nlua_modules_sources,
        .flags = &flags,
    });

    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);

    // Similar to creating the run step earlier, this exposes a `test` step to
    // the `zig build --help` menu, providing a way for the user to request
    // running the unit tests.
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}
