const std = @import("std");
const builtin = @import("builtin");

// Resolve the correct library file names and paths based on the target OS.
fn addNativeLibraries(step: *std.Build.Step.Compile, b: *std.Build) void {
    step.addIncludePath(b.path("../../c_api/include"));
    step.addIncludePath(b.path("../../include"));
    step.linkLibC();
    step.linkLibCpp();

    const target_os = step.rootModuleTarget().os.tag;
    switch (target_os) {
        .windows => {
            // On Windows, link against import libraries (.lib)
            step.addLibraryPath(b.path("../../build/lib"));
            step.linkSystemLibrary("cfwfinder");
            step.linkSystemLibrary("fwfinder");
            // Windows USB enumeration deps
            step.linkSystemLibrary("setupapi");
            step.linkSystemLibrary("cfgmgr32");
        },
        .linux => {
            step.addObjectFile(b.path("../../build/c_api/libcfwfinder.so"));
            step.addObjectFile(b.path("../../build/libfwfinder.so"));
            step.addRPath(b.path("../../build/c_api"));
            step.addRPath(b.path("../../build"));
            step.linkSystemLibrary("udev");
        },
        .macos => {
            step.addObjectFile(b.path("../../build/c_api/libcfwfinder.dylib"));
            step.addObjectFile(b.path("../../build/libfwfinder.dylib"));
            step.addRPath(b.path("../../build/c_api"));
            step.addRPath(b.path("../../build"));
            step.linkFramework("IOKit");
            step.linkFramework("CoreFoundation");
        },
        else => {},
    }
}

// On Windows, DLLs must be on PATH for the process. Add the DLL directory
// to the RunStep so it can find them at runtime.
fn addDllSearchPaths(run_step: *std.Build.Step.Run, b: *std.Build) void {
    if (builtin.os.tag == .windows) {
        run_step.addPathDir(b.path("../../build/bin").getPath(b));
    }
}

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
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

    // This creates a "module", which represents a collection of source files alongside
    // some compilation options, such as optimization mode and linked system libraries.
    // Every executable or library we compile will be based on one or more modules.
    const lib_mod = b.createModule(.{
        // `root_source_file` is the Zig "entry point" of the module. If a module
        // only contains e.g. external object files, you can make this `null`.
        // In this case the main source file is merely a path, however, in more
        // complicated build scripts, this could be a generated file.
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });

    // We will also create a module for our other entry point, 'main.zig'.
    const exe_mod = b.createModule(.{
        // `root_source_file` is the Zig "entry point" of the module. If a module
        // only contains e.g. external object files, you can make this `null`.
        // In this case the main source file is merely a path, however, in more
        // complicated build scripts, this could be a generated file.
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Modules can depend on one another using the `std.Build.Module.addImport` function.
    // This is what allows Zig source code to use `@import("foo")` where 'foo' is not a
    // file path. In this case, we set up `exe_mod` to import `lib_mod`.
    exe_mod.addImport("freewili_finder", lib_mod);

    // Now, we will create a static library based on the module we created above.
    // This creates a `std.Build.Step.Compile`, which is the build step responsible
    // for actually invoking the compiler.
    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "fwf_zig",
        .root_module = lib_mod,
    });

    // Add include paths and link native libraries
    addNativeLibraries(lib, b);

    // This declares intent for the library to be installed into the standard
    // location when the user invokes the "install" step (the default step when
    // running `zig build`).
    b.installArtifact(lib);

    // This creates another `std.Build.Step.Compile`, but this one builds an executable
    // rather than a static library.
    const exe = b.addExecutable(.{
        .name = "fwf_zig",
        .root_module = exe_mod,
    });

    // Add include path for the C API header
    addNativeLibraries(exe, b);

    // This declares intent for the executable to be installed into the
    // standard location when the user invokes the "install" step (the default
    // step when running `zig build`).
    b.installArtifact(exe);

    // This *creates* a Run step in the build graph, to be executed when another
    // step is evaluated that depends on it. The next line below will establish
    // such a dependency.
    const run_cmd = b.addRunArtifact(exe);
    addDllSearchPaths(run_cmd, b);

    // By making the run step depend on the install step, it will be run from the
    // installation directory rather than directly from within the cache directory.
    // This is not necessary, however, if the application depends on other installed
    // files, this ensures they will be present and in the expected location.
    run_cmd.step.dependOn(b.getInstallStep());

    // This allows the user to pass arguments to the application in the build
    // command itself, like this: `zig build run -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // This creates a build step. It will be visible in the `zig build --help` menu,
    // and can be selected like this: `zig build run`
    // This will evaluate the `run` step rather than the default, which is "install".
    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    // Creates a step for unit testing. This only builds the test executable
    // but does not run it.
    const lib_unit_tests = b.addTest(.{
        .root_module = lib_mod,
    });

    // Add the same library configuration to lib_unit_tests as the main lib
    addNativeLibraries(lib_unit_tests, b);

    const run_lib_unit_tests = b.addRunArtifact(lib_unit_tests);
    addDllSearchPaths(run_lib_unit_tests, b);

    const exe_unit_tests = b.addTest(.{
        .root_module = exe_mod,
    });

    // Add the same library configuration to exe_unit_tests as the main exe
    addNativeLibraries(exe_unit_tests, b);

    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
    addDllSearchPaths(run_exe_unit_tests, b);

    // Similar to creating the run step earlier, this exposes a `test` step to
    // the `zig build --help` menu, providing a way for the user to request
    // running the unit tests.
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_lib_unit_tests.step);
    test_step.dependOn(&run_exe_unit_tests.step);
}
