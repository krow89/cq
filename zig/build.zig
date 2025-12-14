const std = @import("std");
const utils = @import("utils.zig");
const tests = @import("build_tests.zig");

pub fn build(b: *std.Build) !void {
    var allocator = std.heap.GeneralPurposeAllocator(.{}).init;
    const gpa = allocator.allocator();
    const trg = b.standardTargetOptions(.{});
    const opt = b.standardOptimizeOption(.{});

    std.debug.print("Building executable\n", .{});
    _ = try utils.buildFor(gpa, &.{ .target = trg, .optimize = opt, .build = b, .artifact_name = "cq" });

    std.debug.print("Building static library\n", .{});
    const libStep = try utils.buildFor(gpa, &.{ .target = trg, .optimize = opt, .build = b, .artifact_name = "cq", .build_lib_info = .{ .excluded_main_src = "main.c", .is_dynamic = false } });

    std.debug.print("Building dynamic library\n", .{});
    _ = try utils.buildFor(gpa, &.{ .target = trg, .optimize = opt, .build = b, .artifact_name = "cq", .build_lib_info = .{ .excluded_main_src = "main.c", .is_dynamic = true } });

    // We want build the test only on linux x86_64 (github action with linux ubuntu runner)
    if (trg.result.os.tag == .linux and trg.result.cpu.arch == .x86_64) {
        std.debug.print("Building tests \n", .{});
        _ = try tests.buildTests(b, libStep, trg, opt);
    } else {
        std.debug.print("Skip uilding tests (only in x86_64-linux target is avaible)", .{});
    }
}
