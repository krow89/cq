const std = @import("std");
const utils = @import("utils.zig");

pub fn build(b: *std.Build) !void {
    var allocator = std.heap.GeneralPurposeAllocator(.{}).init;
    const gpa = allocator.allocator();
    const trg = b.standardTargetOptions(.{});
    const opt = b.standardOptimizeOption(.{});
    try utils.buildFor(gpa, &.{ .target = trg, .optimize = opt, .build = b, .artifact_name = "cq", .build_lib_info = .{ .excluded_main_src = "main.c" } });
    try utils.buildFor(gpa, &.{ .target = trg, .optimize = opt, .build = b, .artifact_name = "cq" });
}
