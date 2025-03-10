#pragma once

#include <stdexcept>
#include <vector>

namespace piomatter {

using matrix_map = std::vector<int>;

enum orientation { normal, r180, ccw, cw };

int orientation_normal(int width, int height, int x, int y) {
    return x + width * y;
}

int orientation_r180(int width, int height, int x, int y) {
    x = width - x - 1;
    y = height - y - 1;
    return orientation_normal(width, height, x, y);
}

int orientation_ccw(int width, int height, int x, int y) {
    return orientation_normal(height, width, y, width - x - 1);
}

int orientation_cw(int width, int height, int x, int y) {
    return orientation_normal(height, width, y - height - 1, x);
}

namespace {
template <typename Cb>
void submap(std::vector<int> &result, int width, int height, int start_x,
            int dx, int count_x_in, int start_y, int dy, int count_y,
            int half_panel_height, const Cb &cb) {

    for (int y = start_y; count_y; count_y -= 2, y += dy) {
        for (int x = start_x, count_x = count_x_in; count_x--; x += dx) {
            result.push_back(cb(width, height, x, y));
            result.push_back(cb(width, height, x, y + dy * half_panel_height));
        }
    }
}
} // namespace

template <typename Cb>
matrix_map make_matrixmap(size_t width, size_t height, size_t n_addr_lines,
                          bool serpentine, const Cb &cb) {

    size_t panel_height = 2 << n_addr_lines;
    if (height % panel_height != 0) {
        throw std::range_error(
            "Overall height does not evenly divide calculated panel height");
    }

    size_t half_panel_height = 1u << n_addr_lines;
    size_t v_panels = height / panel_height;
    size_t pixels_across = width * v_panels;
    matrix_map result;
    result.reserve(width * height);

    for (size_t i = 0; i < half_panel_height; i++) {
        for (size_t j = 0; j < pixels_across; j++) {
            int panel_no = j / width;
            int panel_idx = j % width;
            int x, y0, y1;

            if (serpentine && panel_no % 2) {
                x = width - panel_idx - 1;
                y0 = (panel_no + 1) * panel_height - i - 1;
                y1 = (panel_no + 1) * panel_height - i - half_panel_height - 1;
            } else {
                x = panel_idx;
                y0 = panel_no * panel_height + i;
                y1 = panel_no * panel_height + i + half_panel_height;
            }
            result.push_back(cb(width, height, x, y0));
            result.push_back(cb(width, height, x, y1));
        }
    }

    return result;
}

struct schedule_entry {
    uint32_t shift, active_time;
};

using schedule = std::vector<schedule_entry>;
using schedule_sequence = std::vector<schedule>;

schedule_sequence rescale_schedule(schedule_sequence ss, size_t pixels_across) {
    uint32_t max_active_time = 0;
    for (auto &s : ss) {
        for (auto &ent : s) {
            max_active_time = std::max(ent.active_time, max_active_time);
        }
    }
    if (max_active_time == 0 || max_active_time >= pixels_across) {
        return ss;
    }
    int scale = (pixels_across + max_active_time - 1) / max_active_time;
    for (auto &s : ss) {
        for (auto &ent : s) {
            ent.active_time *= scale;
        }
    }
    return ss;
}

schedule_sequence make_simple_schedule(int n_planes, size_t pixels_across) {
    if (n_planes < 1 || n_planes > 10) {
        throw std::range_error("n_planes out of range");
    }
    schedule result;
    for (int i = 0; i < n_planes; i++) {
        result.emplace_back(9 - i, (1 << (n_planes - i - 1)));
    }
    return rescale_schedule({result}, pixels_across);
}

// Make a temporal dither schedule. All the top `n_planes` are shown everytime,
// but the lowest planes are done in a cycle of `n_temporal_planes`:
//   2: {0, 1}; 4: {0, 1, 2, 3}
schedule_sequence make_temporal_dither_schedule(int n_planes,
                                                size_t pixels_across,
                                                int n_temporal_planes) {
    if (n_planes < 1 || n_planes > 10) {
        throw std::range_error("n_planes out of range");
    }
    if (n_temporal_planes < 2) {
        // either 0 or 1 temporal planes are not really temporal at all
        return make_simple_schedule(n_planes, pixels_across);
    }
    if (n_temporal_planes >= n_planes) {
        throw std::range_error("n_temporal_planes can't exceed n_planes");
    }
    if (n_temporal_planes != 2 && n_temporal_planes != 4) {
        // the code can generate a schedule for 8 temporal planes, but it
        // flickers intolerably
        throw std::range_error("n_temporal_planes must be 0, 1, 2, or 4");
    }

    int n_real_planes = n_planes - n_temporal_planes;

    schedule base_sched;
    for (int j = 0; j < n_real_planes; j++) {
        base_sched.emplace_back(
            9 - j, (1 << (n_temporal_planes + n_real_planes - j - 1)) /
                       n_temporal_planes);
    }

    schedule_sequence result;

    auto add_sched = [&result, &base_sched](int plane, int count) {
        auto sched = base_sched;
        sched.emplace_back(9 - plane, count);
        result.emplace_back(sched);
    };

    for (int i = 0; i < n_temporal_planes; i++) {
        add_sched(n_real_planes + i, 1 << (n_temporal_planes - i - 1));
    }
#if 0
    std::vector<uint32_t> counts(10, 0);
    for (auto s : result) {
        for(auto t: s) {
            counts[t.shift] += t.active_time;
        }
    }
    for (auto s : counts) {
        printf("%d ", s);
    }
    printf("\n");
#endif

    return rescale_schedule(result, pixels_across);
    ;
}

struct matrix_geometry {
    template <typename Cb>
    matrix_geometry(size_t pixels_across, size_t n_addr_lines, int n_planes,
                    int n_temporal_planes, size_t width, size_t height,
                    bool serpentine, const Cb &cb)
        : matrix_geometry(
              pixels_across, n_addr_lines, n_planes, n_temporal_planes, width,
              height,
              make_matrixmap(width, height, n_addr_lines, serpentine, cb), 2) {}

    matrix_geometry(size_t pixels_across, size_t n_addr_lines, int n_planes,
                    int n_temporal_planes, size_t width, size_t height,
                    matrix_map map, size_t n_lanes)
        : matrix_geometry(pixels_across, n_addr_lines, width, height, map,
                          n_lanes,
                          make_temporal_dither_schedule(n_planes, pixels_across,
                                                        n_temporal_planes)) {}

    matrix_geometry(size_t pixels_across, size_t n_addr_lines, size_t width,
                    size_t height, matrix_map map, size_t n_lanes,
                    const schedule_sequence &schedules)
        : pixels_across(pixels_across), n_addr_lines(n_addr_lines),
          n_lanes(n_lanes), width(width), height(height),
          map(map), schedules{schedules} {
        size_t pixels_down = n_lanes << n_addr_lines;
        if (map.size() != pixels_down * pixels_across) {
            throw std::range_error(
                "map size does not match calculated pixel count");
        }
    }

    size_t pixels_across, n_addr_lines, n_lanes;
    size_t width, height;
    matrix_map map;
    schedule_sequence schedules;
};
} // namespace piomatter
