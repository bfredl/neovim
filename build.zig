const std = @import("std");

// TODO: this should be suggested to upstream
pub fn run_preprocessor(b: *std.Build, src: std.Build.LazyPath, output_name: []const u8, include_dirs: []const []const u8, c_macros: []const []const u8) std.Build.LazyPath {
    const run_step = std.Build.Step.Run.create(b, b.fmt("preprocess to get {s}", .{output_name}));
    run_step.addArgs(&.{ b.graph.zig_exe, "cc", "-E" });
    run_step.addFileArg(src);
    run_step.addArg("-o");
    const output = run_step.addOutputFileArg(output_name);
    // TODO: include path logic for addCSourceFiles and TranslateC is _very_ different
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

pub fn generate_header_for(b: *std.Build, name: []const u8, nlua0: *std.Build.Step.Compile) !*std.Build.Step.Run {
    if (name.len < 2 or !std.mem.eql(u8, ".c", name[name.len - 2 ..])) return error.InvalidBaseName;
    const basename = name[0 .. name.len - 2];
    // TODO: need .deps/usr/include and zig_lua include dirs
    const input_file = b.path(b.fmt("src/nvim/{s}", .{name}));
    const i_file = run_preprocessor(b, input_file, b.fmt("{s}.i", .{basename}), &.{ "src", "src/includes_fixmelater" }, &.{ "HAVE_UNIBILIUM", "_GNU_SOURCE" });
    const run_step = b.addRunArtifact(nlua0);
    run_step.addFileArg(b.path("src/nvim/generators/gen_declarations.lua"));
    run_step.addFileArg(input_file);
    _ = run_step.addOutputFileArg(b.fmt("{s}.c.generated.h", .{basename}));
    _ = run_step.addOutputFileArg(b.fmt("{s}.h.generated.h", .{basename}));
    run_step.addFileArg(i_file);
    return run_step;
}

pub fn build(b: *std.Build) !void {
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

    const lpeg = b.dependency("lpeg", .{});

    const nlua0_exe = b.addExecutable(.{
        .name = "nlua0",
        .root_source_file = .{ .path = "src/nlua0.zig" },
        .target = target,
        .optimize = optimize,
    });
    const nlua0_mod = &nlua0_exe.root_module;

    const exe_unit_tests = b.addTest(.{
        .root_source_file = .{ .path = "src/nlua0.zig" },
        .target = target,
        .optimize = optimize,
    });

    // TODO: need mod.addLibraryPathFromDependency(ziglua)
    // nlua0_mod.include_dirs.append(nlua0_mod.owner.allocator, .{ .other_step = ziglua_mod }) catch @panic("OOM");

    const flags = [_][]const u8{
        // Standard version used in Lua Makefile
        "-std=gnu99",
    };

    for ([2]*std.Build.Module{ nlua0_mod, &exe_unit_tests.root_module }) |mod| {
        mod.addImport("ziglua", ziglua.module("ziglua"));
        mod.addImport("embedded_data", embedded_data);
        // addImport already links by itself. but we need headers as well..
        mod.linkLibrary(ziglua.artifact("lua"));

        mod.addIncludePath(b.path("src"));
        mod.addIncludePath(b.path("src/includes_fixmelater"));
        mod.addIncludePath(lpeg.path(""));
        mod.addCSourceFiles(.{
            .files = &.{
                "src/mpack/lmpack.c",
                "src/mpack/mpack_core.c",
                "src/mpack/object.c",
                "src/mpack/conv.c",
                "src/mpack/rpc.c",
            },
            .flags = &flags,
        });
        mod.addCSourceFiles(.{
            .root = .{ .dependency = .{ .dependency = lpeg, .sub_path = "" } },
            .files = &.{
                "lpcap.c",
                "lpcode.c",
                "lpprint.c",
                "lptree.c",
                "lpvm.c",
            },
            .flags = &flags,
        });
    }

    // for debugging the nlua0 environment
    // like this: `zig build nlua0 -- script.lua {args}`
    const run_cmd = b.addRunArtifact(nlua0_exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step("nlua0", "Run nlua0 build tool");
    run_step.dependOn(&run_cmd.step);

    const wip_step = b.step("wip", "rearrange the power of it all");

    const gen_step = try generate_header_for(b, "autocmd.c", nlua0_exe);
    wip_step.dependOn(&gen_step.step);

    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);

    // Similar to creating the run step earlier, this exposes a `test` step to
    // the `zig build --help` menu, providing a way for the user to request
    // running the unit tests.
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}
