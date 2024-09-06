#ifndef CORE_STRUCT_H
#define CORE_STRUCT_H

#include <iostream>
#include <memory>
#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <iterator>
#include <functional>


using namespace std;

#define PRINT_FNC(func_name) cout << "[" << func_name << "] "
#define DRAM_READ_COST 1.0f
#define NEIGHBOUR_READ_COST 0.5f

struct CoreArgs {
    int core_id;
    shared_ptr<int> num_dram_read;
    shared_ptr<int> num_unit_compute;
    shared_ptr<int> num_neighbour_read;
    string name;

    CoreArgs() : 
        core_id(-1), 
        num_dram_read(make_shared<int>(0)),
        num_neighbour_read(make_shared<int>(0)),
        num_unit_compute(make_shared<int>(0))
    {}
    CoreArgs(int core_id) : 
        core_id(core_id), 
        num_dram_read(make_shared<int>(0)), 
        num_neighbour_read(make_shared<int>(0)),
        num_unit_compute(make_shared<int>(0))
    {}
    
    string str() const { return name.empty() ? "CoreArgs" : "CoreArgs::" + name; }

    void set_core_id(int core_id) { this->core_id = core_id; }
    void set_name(string name) { this->name = name; }
    void dram_read(int num = 1) { (*num_dram_read)+=num; }
    void neighboard_read(int num = 1) { (*num_neighbour_read)+=num; }
    void unit_compute(int num = 1) { (*num_unit_compute)+=num; }
    bool is_name(string name) { return this->name == name; }

    void print_stats() {
        float ratio;
        int total = *num_dram_read * DRAM_READ_COST + *num_neighbour_read * NEIGHBOUR_READ_COST;
        if (total == 0) {
            ratio = 0;
        } else 
            ratio = static_cast<float>(*num_unit_compute) / total;
        string print_name = this->str();
        PRINT_FNC(print_name) << "num_dram_read: " << *num_dram_read << endl;
        PRINT_FNC(print_name) << "num_neighbour_read: " << *num_neighbour_read << endl;
        PRINT_FNC(print_name) << "num_unit_compute: " << *num_unit_compute << endl;
        PRINT_FNC(print_name) << "arithmetic ratio: " << ratio << endl;
    }
};

struct CoreCoord {
    constexpr CoreCoord() : x{}, y{} {}
    constexpr CoreCoord(uint32_t x, uint32_t y) : x(x), y(y) {}

    uint32_t x;
    uint32_t y;

    string str() const { return "(x=" + to_string(x) + ",y=" + to_string(y) + ")"; }
};


inline bool operator==(const CoreCoord &a, const CoreCoord &b) { return a.x == b.x && a.y == b.y; }

inline bool operator!=(const CoreCoord &a, const CoreCoord &b) { return !(a == b); }

inline bool operator<(const CoreCoord &left, const CoreCoord &right) {
    return (left.x < right.x || (left.x == right.x && left.y < right.y));
}

inline bool operator<=(const CoreCoord &a, const CoreCoord &b) { return (a < b) or (a == b); }

struct CoreRange {
    CoreCoord start_coord;
    CoreCoord end_coord;
    CoreRange(const CoreCoord &point) {
        this->start_coord = point;
        this->end_coord = point;
    }

    CoreRange(const CoreCoord &start_coord, const CoreCoord &end_coord) {
        if(end_coord.x < start_coord.x || end_coord.y < start_coord.y)
        {
            cout << "ERROR: Invalid core range for start_coord: " << start_coord.str() << ", end_coord: " << end_coord.str() + "" << endl;
            exit(1);
        }

        this->start_coord = start_coord;
        this->end_coord = end_coord;
    }

    CoreRange(const CoreRange &other) = default;
    CoreRange &operator=(const CoreRange &other) = default;
    CoreRange(CoreRange &&other) = default;
    CoreRange &operator=(CoreRange &&other) = default;

    // void validate() {
    //     TT_FATAL(
    //         end_coord.x >= start_coord.x and end_coord.y >= start_coord.y,
    //         "Invalid core range for start_coord: {}, end_coord: {}", start_coord.str(), end_coord.str());
    // }

    inline bool adjacent(const CoreRange &other) const {
        size_t x1 = max(this->start_coord.x, other.start_coord.x);
        size_t y1 = max(this->start_coord.y, other.start_coord.y);
        size_t x2 = min(this->end_coord.x, other.end_coord.x);
        size_t y2 = min(this->end_coord.y, other.end_coord.y);
        return ((x2 + 1 == x1 && y1 <= y2) || (y2 + 1 == y1 && x1 <= x2));
    }

    inline bool contains(const CoreRange &other) const {
        return (other.start_coord.x >= this->start_coord.x) && (other.end_coord.x <= this->end_coord.x) && (other.start_coord.y >= this->start_coord.y) &&
               (other.end_coord.y <= this->end_coord.y);
    }

    inline bool contains(const CoreCoord &other) const {
        return (other.x >= this->start_coord.x) && (other.x <= this->end_coord.x) && (other.y >= this->start_coord.y) &&
               (other.y <= this->end_coord.y);
    }

    string str() const { return "[" + this->start_coord.str() + " - " + this->end_coord.str() + "]"; }

    size_t size() const { return (this->end_coord.x - this->start_coord.x + 1) * (this->end_coord.y - this->start_coord.y + 1); }

    CoreCoord grid_size() const { return {this->end_coord.x - this->start_coord.x + 1, this->end_coord.y - this->start_coord.y + 1}; }

};


inline bool operator==(const CoreRange &a, const CoreRange &b) {
    return a.start_coord == b.start_coord && a.end_coord == b.end_coord;
}

inline bool operator!=(const CoreRange &a, const CoreRange &b) { return !(a == b); }

inline bool operator<(const CoreRange &left, const CoreRange &right) {
    return (left.start_coord < right.start_coord || (left.start_coord == right.start_coord && left.end_coord < right.end_coord));
}


class CoreRangeSet {
   public:
    CoreRangeSet(const set<CoreRange> &core_ranges) : ranges_(core_ranges) {
        for (auto outer_it = this->ranges_.begin(); outer_it != this->ranges_.end(); outer_it++) {
            for (auto inner_it = this->ranges_.begin(); inner_it != this->ranges_.end(); inner_it++) {
                if (outer_it == inner_it) {
                    continue;
                }
                CoreRange first_core_range = *outer_it;
                CoreRange second_core_range = *inner_it;
                bool first_core_left_of_second = first_core_range.end_coord.x < second_core_range.start_coord.x;
                bool first_core_right_of_second = first_core_range.start_coord.x > second_core_range.end_coord.x;
                bool first_core_above_second = first_core_range.end_coord.y < second_core_range.start_coord.y;
                bool first_core_below_second = first_core_range.start_coord.y > second_core_range.end_coord.y;
                auto no_overlap = first_core_left_of_second or first_core_right_of_second or first_core_above_second or
                                  first_core_below_second;
                if (not no_overlap) {
                    // TT_THROW(("Cannot create CoreRangeSet with specified core ranges because core ranges " +
                    //           first_core_range.str() + " and " + second_core_range.str() + " overlap!")
                    //              .c_str());
                }
            }
        }
    }

    CoreRangeSet(const CoreRangeSet &other) = default;
    CoreRangeSet &operator=(const CoreRangeSet &other) = default;

    CoreRangeSet(CoreRangeSet &&other) = default;
    CoreRangeSet &operator=(CoreRangeSet &&other) = default;

    auto size() const { return ranges_.size(); }

    string str() const { 
        string core_range_set_str = "{";
        for (const auto &core_range : this->ranges_) {
            core_range_set_str += core_range.str() + ", ";
        }
        core_range_set_str[core_range_set_str.length() - 2] = '}';
        core_range_set_str.pop_back();
        return core_range_set_str;
     }

    // CoreRangeSet merge(const set<CoreRange> &other) const {
    //     size_t min_x = numeric_limits<size_t>::max(), max_x = 0, min_y = numeric_limits<size_t>::max(),
    //            max_y = 0;
    //     set<CoreRange> crs = this->ranges_;
    //     crs.insert(other.begin(), other.end());

    //     for (const auto &cr : crs) {
    //         // cout << "merging " << cr.str() << endl;
    //         min_x = min(min_x, cr.start_coord.x);
    //         max_x = max(max_x, cr.end_coord.x);
    //         min_y = min(min_y, cr.start_coord.y);
    //         max_y = max(max_y, cr.end_coord.y);
    //     }

    //     // By overallocating by one x entry, we can avoid needing to check for
    //     // boundary conditions when iterating, since there'll always be one
    //     // last false entry
    //     bool grid[max_y + 1][max_x + 2];
    //     memset(grid, 0, sizeof(grid));

    //     for (const auto &cr : crs)
    //         for (unsigned y = cr.start_coord.y; y <= cr.end_coord.y; y++)
    //             for (unsigned x = cr.start_coord.x; x <= cr.end_coord.x; x++) grid[y][x] = true;

    //     crs.clear();
    //     for (unsigned y = min_y; y <= max_y; y++) {
    //         set<CoreRange> filter_set, tmp, new_crs;
    //         vector<CoreRange> ranges;
    //         for (unsigned x = min_x; x <= max_x + 1; x++) {
    //             if (grid[y][x]) {
    //                 unsigned x_start = x;
    //                 while (grid[y][x]) x++;
    //                 ranges.push_back(CoreRange({x_start, y}, {x - 1, y}));
    //             }
    //         }

    //         for (const auto &cr : ranges) {
    //             for (const auto &prev_cr : crs) {
    //                 if (auto merged = cr.merge(prev_cr)) {
    //                     // cout << "merging " << cr.str() << " and " << prev_cr.str() << " with " <<
    //                     // merged.value().str() << endl;
    //                     new_crs.insert(merged.value());
    //                     filter_set.insert(prev_cr);
    //                     filter_set.insert(cr);
    //                 }
    //             }
    //             crs.insert(cr);
    //         }
    //         // Set(A) = Set(A) - Set(B)
    //         set_difference(
    //             make_move_iterator(crs.begin()),
    //             make_move_iterator(crs.end()),
    //             filter_set.begin(),
    //             filter_set.end(),
    //             inserter(tmp, tmp.end()));
    //         crs.swap(tmp);
    //         crs.insert(new_crs.begin(), new_crs.end());
    //     }
    //     // for ( const auto & cr : crs ){
    //     //   cout << " final merged CR:" << cr.str() << endl;
    //     // }
    //     return CoreRangeSet(crs);
    // }

    // CoreRangeSet merge(const CoreRangeSet &s) const { return this->merge(s.ranges()); }

    // inline bool core_coord_in_core_ranges(const CoreCoord &core_coord) const {
    //     for (const auto &cr : this->ranges_) {
    //         if (cr.contains(core_coord))
    //             return true;
    //     }
    //     return false;
    // }

    // inline bool intersects(const CoreRange &cr) const {
    //     for (const auto &local_cr : this->ranges_) {
    //         if (local_cr.intersects(cr))
    //             return true;
    //     }
    //     return false;
    // }

    const set<CoreRange> &ranges() const { return this->ranges_; }

    const uint32_t num_cores() const {
        uint32_t num_cores = 0;
        for (const auto &core_range : this->ranges()) {
            num_cores += core_range.size();
        }
        return num_cores;
    }

    // CoreRange bounding_box() const {
    //     TT_FATAL(this->ranges().size() > 0, "Cannot get bounding_box of an empty CoreRangeSet!");
    //     size_t min_x = UINT32_MAX, min_y = UINT32_MAX, max_x = 0, max_y = 0;
    //     for (const auto &cr : this->ranges()) {
    //         min_x = min(min_x, cr.start_coord.x);
    //         max_x = max(max_x, cr.end_coord.x);
    //         min_y = min(min_y, cr.start_coord.y);
    //         max_y = max(max_y, cr.end_coord.y);
    //     }
    //     return {{min_x, min_y}, {max_x, max_y}};
    // }

   private:
    set<CoreRange> ranges_;
};

// const inline bool operator==(const CoreRangeSet &a, const CoreRangeSet &b) {
//     if (a.ranges().size() == b.ranges().size()) {
//         auto range_a = a.ranges();
//         auto range_b = b.ranges();
//         for (auto it_a = range_a.begin(), it_b = range_b.begin(); it_a != range_a.end(); it_a++, it_b++) {
//             if (*it_a != *it_b) {
//                 return false;
//             }
//         }
//         return true;
//     }
//     return false;
// }

// const inline bool operator!=(const CoreRangeSet &a, const CoreRangeSet &b) { return !(a == b); }


#endif // CORE_STRUCT_H