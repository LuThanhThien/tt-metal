#ifndef CORE_STRUCT_H
#define CORE_STRUCT_H

#include <iostream>
#include <memory>

using namespace std;

#define PRINT_FNC(func_name) std::cout << "[" << func_name << "] "
#define DRAM_READ_COST 1.0f
#define NEIGHBOUR_READ_COST 0.5f

struct CoreArgs {
    int core_id;
    std::shared_ptr<int> num_dram_read;
    std::shared_ptr<int> num_unit_compute;
    std::shared_ptr<int> num_neighbour_read;
    string name;

    

    CoreArgs() : 
        core_id(-1), 
        num_dram_read(std::make_shared<int>(0)),
        num_neighbour_read(std::make_shared<int>(0)),
        num_unit_compute(std::make_shared<int>(0))
    {}
    CoreArgs(int core_id) : 
        core_id(core_id), 
        num_dram_read(std::make_shared<int>(0)), 
        num_neighbour_read(std::make_shared<int>(0)),
        num_unit_compute(std::make_shared<int>(0))
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

    std::string str() const { return "(x=" + std::to_string(x) + ",y=" + std::to_string(y) + ")"; }
};


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
        std::size_t x1 = std::max(this->start_coord.x, other.start_coord.x);
        std::size_t y1 = std::max(this->start_coord.y, other.start_coord.y);
        std::size_t x2 = std::min(this->end_coord.x, other.end_coord.x);
        std::size_t y2 = std::min(this->end_coord.y, other.end_coord.y);
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

    std::string str() const { return "[" + this->start_coord.str() + " - " + this->end_coord.str() + "]"; }

    size_t size() const { return (this->end_coord.x - this->start_coord.x + 1) * (this->end_coord.y - this->start_coord.y + 1); }

    CoreCoord grid_size() const { return {this->end_coord.x - this->start_coord.x + 1, this->end_coord.y - this->start_coord.y + 1}; }

};


#endif // CORE_STRUCT_H