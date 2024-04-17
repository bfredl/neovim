const std = @import("std");

// TODO: this should be suggested to upstream
pub fn run_preprocessor(b: *std.Build, src: std.Build.LazyPath, output_name: []const u8, include_dirs: []const []const u8, c_macros: []const []const u8) std.Build.LazyPath {
    const run_step = std.Build.Step.Run.create(b, b.fmt("preprocess to get {s}", .{output_name}));
    run_step.addArgs(&.{ b.graph.zig_exe, "cc", "-E" });
    run_step.addFileArg(src);
    run_step.addArg("-o");
    const output = run_step.addOutputFileArg(output_name);
    // TODO: arg logic for addCSourceFiles and TranslateC is _very_ different
    for (include_dirs) |include_dir| {
        run_step.addArg("-I");
        run_step.addArg(include_dir);
    }
    for (c_macros) |c_macro| {
        run_step.addArg(b.fmt("-D{s}", .{c_macro}));
        run_step.addArg(c_macro);
    }
    run_step.addArgs(&.{ "-MMD", "-MF" });
    _ = run_step.addDepFileOutputArg(b.fmt("{s}.d", .{output_name}));
    return output;
}

pub fn generate_header_for(b: *std.Build, name: []u8, nlua0: *std.Build.LazyPath) !*std.Build.Step.Compile {
    const i_file = run_preprocessor(b, b.path(name), b.fmt("{}.d", .{name}), &.{ "src", "src/includes_fixmelater" }, &.{ "HAVE_UNIBILIUM", "_GNU_SOURCE" });
    const run_step = b.addRunArtifact(nlua0);
    run_step.addFileArg(b.path("src/nvim/generators/gen_declarations.lua"));
    run_step.addFilearg(b.path(name));
    run_step.addOutputFileArg(b.fmt("{s}.generated.c", .{name}));
    run_step.addOutputFileArg(b.fmt("{s}.generated.h", .{name}));
    run_step.addFilearg(i_file);
    return run_step;
}

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

    // TODO: need .deps/usr/include and zig_lua include dirs
    const i_file = run_preprocessor(b, b.path("src/nvim/autocmd.c"), "nvim/autocmd.i", &.{ "src", "src/includes_fixmelater" }, &.{ "HAVE_UNIBILIUM", "_GNU_SOURCE" });
    const wip_step = b.step("wip", "rearrange the power of it all");
    wip_step.dependOn(i_file.generated.step);

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
