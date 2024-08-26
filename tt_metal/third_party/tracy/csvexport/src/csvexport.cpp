#ifdef _WIN32
#  include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "../../server/TracyFileRead.hpp"
#include "../../server/TracyWorker.hpp"
#include "../../getopt/getopt.h"

void print_usage_exit(int e)
{
    fprintf(stderr, "Extract statistics from a trace to a CSV format\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  extract [OPTION...] <trace file>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -h, --help        Print usage\n");
    fprintf(stderr, "  -f, --filter arg  Filter zone names (default: "")\n");
    fprintf(stderr, "  -x arg            List of special functions delimited by comma (default: "")\n");
    fprintf(stderr, "  -p arg            Parent to find for special functions (default: "")\n");
    fprintf(stderr, "  -s, --sep arg     CSV separator (default: ,)\n");
    fprintf(stderr, "  -c, --case        Case sensitive filtering\n");
    fprintf(stderr, "  -e, --self        Get self times\n");
    fprintf(stderr, "  -u, --unwrap      Report each zone event\n");
    fprintf(stderr, "  -m, --messages    Report only messages\n");

    exit(e);
}

struct Args {
    const char* filter;
    const char* separator;
    const char* trace_file;
    const char* special_functions;
    const char* special_parent_function;
    bool case_sensitive;
    bool self_time;
    bool unwrap;
    bool unwrapMessages;
};

Args parse_args(int argc, char** argv)
{
    if (argc == 1)
    {
        print_usage_exit(1);
    }

    Args args = { "", ",", "", "", "", false, false, false, false };

    struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "filter", optional_argument, NULL, 'f' },
        { "sep", optional_argument, NULL, 's' },
        { "case", no_argument, NULL, 'c' },
        { "self", no_argument, NULL, 'e' },
        { "unwrap", no_argument, NULL, 'u' },
        { "messages", no_argument, NULL, 'm' },
        { "special_functions", no_argument, NULL, 'x' },
        { "special_parent_function", no_argument, NULL, 'p' },
        { NULL, 0, NULL, 0 }
    };

    int c;
    while ((c = getopt_long(argc, argv, "hf:p:x:s:ceum", long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 'h':
            print_usage_exit(0);
            break;
        case 'f':
            args.filter = optarg;
            break;
        case 's':
            args.separator = optarg;
            break;
        case 'p':
            args.special_parent_function = optarg;
            break;
        case 'x':
            args.special_functions = optarg;
            break;
        case 'c':
            args.case_sensitive = true;
            break;
        case 'e':
            args.self_time = true;
            break;
        case 'u':
            args.unwrap = true;
            break;
        case 'm':
            args.unwrapMessages = true;
            break;
        default:
            print_usage_exit(1);
            break;
        }
    }

    if (argc != optind + 1)
    {
        print_usage_exit(1);
    }

    args.trace_file = argv[optind];

    return args;
}

bool is_substring(
    const char* term,
    const char* s,
    bool case_sensitive = false
){
    auto new_term = std::string(term);
    auto new_s = std::string(s);

    if (!case_sensitive) {
        std::transform(
            new_term.begin(),
            new_term.end(),
            new_term.begin(),
            [](unsigned char c){ return std::tolower(c); }
        );

        std::transform(
            new_s.begin(),
            new_s.end(),
            new_s.begin(),
            [](unsigned char c){ return std::tolower(c); }
        );
    }

    return new_s.find(new_term) != std::string::npos;
}

const char* get_name(int32_t id, const tracy::Worker& worker)
{
    auto& srcloc = worker.GetSourceLocation(id);
    return worker.GetString(srcloc.name.active ? srcloc.name : srcloc.function);
}

template <typename T>
std::string join(const T& v, const char* sep) {
    std::ostringstream s;
    for (const auto& i : v) {
        if (&i != &v[0]) {
            s << sep;
        }
        s << i;
    }
    return s.str();
}

//From TracyView_Utility.cpp
const tracy::ZoneEvent* GetZoneParent(const tracy::Worker& worker, const tracy::ZoneEvent& zone, uint64_t tid )
{
    const auto& thread = worker.GetThreadData(tid);
    if( thread == nullptr )
    {
        return nullptr;
    }
    const tracy::ZoneEvent* parent = nullptr;
    const tracy::Vector<tracy::short_ptr<tracy::ZoneEvent>>* timeline = &thread->timeline;
    if( timeline == nullptr || timeline->empty() ) return nullptr;
    for(;;)
    {
        if( timeline->is_magic() )
        {
            auto vec = (tracy::Vector<tracy::ZoneEvent>*)timeline;
            auto it = std::upper_bound( vec->begin(), vec->end(), zone.Start(), [] ( const auto& l, const auto& r ) { return l < r.Start(); } );
            if( it != vec->begin() ) --it;
            if( zone.IsEndValid() && it->Start() > zone.End() ) break;
            if( it == &zone ) return parent;
            if( !it->HasChildren() ) break;
            parent = it;
            timeline = &worker.GetZoneChildren( parent->Child() );
        }
        else
        {
            auto it = std::upper_bound( timeline->begin(), timeline->end(), zone.Start(), [] ( const auto& l, const auto& r ) { return l < r->Start(); } );
            if( it != timeline->begin() ) --it;
            if( zone.IsEndValid() && (*it)->Start() > zone.End() ) break;
            if( *it == &zone ) return parent;
            if( !(*it)->HasChildren() ) break;
            parent = *it;
            timeline = &worker.GetZoneChildren( parent->Child() );
        }
    }
    return nullptr;
}

const tracy::ZoneEvent* GetSpecialParent(const tracy::Worker& worker, const tracy::ZoneEvent& zone_event, uint64_t tid, std::string& functionName)
{
    const auto parent_zone_event = GetZoneParent(worker, zone_event, tid);
    if (parent_zone_event != nullptr)
    {
        std::string parent_zone_name = get_name(parent_zone_event->SrcLoc(), worker);
        std::string parent_zone_text = "";
        if (parent_zone_name.find(functionName) != std::string::npos)
        {
            return parent_zone_event;
        }
        return GetSpecialParent(worker, *parent_zone_event, tid, functionName);
    }
    return nullptr;
}

// From TracyView.cpp
int64_t GetZoneChildTimeFast(
    const tracy::Worker& worker,
    const tracy::ZoneEvent& zone
){
    int64_t time = 0;
    if( zone.HasChildren() )
    {
        auto& children = worker.GetZoneChildren( zone.Child() );
        if( children.is_magic() )
        {
            auto& vec = *(tracy::Vector<tracy::ZoneEvent>*)&children;
            for( auto& v : vec )
            {
                assert( v.IsEndValid() );
                time += v.End() - v.Start();
            }
        }
        else
        {
            for( auto& v : children )
            {
                assert( v->IsEndValid() );
                time += v->End() - v->Start();
            }
        }
    }
    return time;
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
    }
#endif

    Args args = parse_args(argc, argv);

    auto f = std::unique_ptr<tracy::FileRead>(
        tracy::FileRead::Open(args.trace_file)
    );
    if (!f)
    {
        fprintf(stderr, "Could not open file %s\n", args.trace_file);
        return 1;
    }

    auto worker = tracy::Worker(*f);

    if (args.unwrapMessages)
    {
        const auto& msgs = worker.GetMessages();

        if (msgs.size() > 0)
        {
            std::vector<const char*> columnsForMessages;
            columnsForMessages = {
                    "MessageName", "total_ns"
                };
            std::string headerForMessages = join(columnsForMessages, args.separator);
            printf("%s\n", headerForMessages.data());

            for(auto& it : msgs)
            {
                std::vector<std::string> values(columnsForMessages.size());

                values[0] = worker.GetString(it->ref);
                values[1] = std::to_string(it->time);

                std::string row = join(values, args.separator);
                printf("%s\n", row.data());
            }
        }
        else
        {
            printf("There are currently no messages!\n");
        }

        return 0;
    }

    while (!worker.AreSourceLocationZonesReady())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto& slz = worker.GetSourceLocationZones();
    auto& slzg = worker.GetGpuSourceLocationZones();
    tracy::Vector<decltype(slz.begin())> slz_selected;
    tracy::Vector<decltype(slzg.begin())> slzg_selected;
    slz_selected.reserve(slz.size());
    slzg_selected.reserve(slzg.size());

    uint32_t total_cnt = 0;
    for(auto it = slz.begin(); it != slz.end(); ++it)
    {
        if(it->second.total != 0)
        {
            ++total_cnt;
            if(args.filter[0] == '\0')
            {
                slz_selected.push_back_no_space_check(it);
            }
            else
            {
                auto name = get_name(it->first, worker);
                if(is_substring(args.filter, name, args.case_sensitive))
                {
                    slz_selected.push_back_no_space_check(it);
                }
            }
        }
    }

    std::vector<std::string> specialFunctions = {};
    std::string specialParent = args.special_parent_function;
    if (specialParent.size() > 0)
    {
        size_t pos = 0;
        std::string token;
        std::string delimiter = ",";
        std::string functionsStr = args.special_functions;
        while ((pos = functionsStr.find(delimiter)) != std::string::npos) {
            token = functionsStr.substr(0, pos);
            specialFunctions.push_back(token);
            functionsStr.erase(0, pos + delimiter.length());
        }
        specialFunctions.push_back(functionsStr);
    }

    std::vector<std::string> columns;
    if (args.unwrap)
    {
        columns = {
            "name", "src_file", "src_line", "zone_name", "zone_text", "ns_since_start", "exec_time_ns", "thread", "special_parent_text"
        };
    }
    else
    {
        columns = {
            "name", "src_file", "src_line", "total_ns", "total_perc",
            "counts", "mean_ns", "min_ns", "max_ns", "std_ns"
        };
    }
    std::string header = join(columns, args.separator);
    printf("%s\n", header.data());

    const auto last_time = worker.GetLastTime();
    for(auto& it : slz_selected)
    {
        std::vector<std::string> values(columns.size());

        values[0] = get_name(it->first, worker);

        const auto& srcloc = worker.GetSourceLocation(it->first);
        values[1] = worker.GetString(srcloc.file);
        values[2] = std::to_string(srcloc.line);

        const auto& zone_data = it->second;

        if (args.unwrap)
        {
            for (const auto& zone_thread_data : zone_data.zones) {
                const auto zone_event = zone_thread_data.Zone();
                const auto tId = zone_thread_data.Thread();

                if (worker.HasZoneExtra(*zone_event))
                {
                    auto extra = worker.GetZoneExtra(*zone_event);
                    if (extra.name.Active())
                    {
                        values[3] = worker.GetString(extra.name);
                    }
                    if (extra.text.Active())
                    {
                        values[4] = "\"" + (std::string)worker.GetString(extra.text) + "\"";
                    }
                }

                const auto start = zone_event->Start();
                const auto end = zone_event->End();

                values[5] = std::to_string(start);

                auto timespan = end - start;
                if (args.self_time) {
                    timespan -= GetZoneChildTimeFast(worker, *zone_event);
                }
                values[6] = std::to_string(timespan);
                values[7] = std::to_string(tId);

                for (auto& function: specialFunctions)
                {
                    if (function == values[0])
                    {
                        const auto special_parent_zone_event = GetSpecialParent(worker, *zone_event, worker.DecompressThread(tId), specialParent);
                        if (special_parent_zone_event != nullptr)
                        {
                            if (worker.HasZoneExtra(*special_parent_zone_event))
                            {
                                auto extra = worker.GetZoneExtra(*special_parent_zone_event);
                                if (extra.text.Active())
                                {
                                    values[8] = worker.GetString(extra.text);
                                }
                            }
                        }
                    }
                }
                std::string row = join(values, args.separator);
                printf("%s\n", row.data());
            }
        }
        else
        {
            const auto time = args.self_time ? zone_data.selfTotal : zone_data.total;
            values[3] = std::to_string(time);
            values[4] = std::to_string(100. * time / last_time);

            values[5] = std::to_string(zone_data.zones.size());

            const auto avg = (args.self_time ? zone_data.selfTotal : zone_data.total)
                / zone_data.zones.size();
            values[6] = std::to_string(avg);

            const auto tmin = args.self_time ? zone_data.selfMin : zone_data.min;
            const auto tmax = args.self_time ? zone_data.selfMax : zone_data.max;
            values[7] = std::to_string(tmin);
            values[8] = std::to_string(tmax);

            const auto sz = zone_data.zones.size();
            const auto ss = zone_data.sumSq
                - 2. * zone_data.total * avg
                + avg * avg * sz;
            double std = 0;
            if( sz > 1 )
                std = sqrt(ss / (sz - 1));
            values[9] = std::to_string(std);

            std::string row = join(values, args.separator);
            printf("%s\n", row.data());
        }
    }

    for(auto& it : slzg_selected)
    {
        std::vector<std::string> values(columns.size());

        values[0] = get_name(it->first, worker);

        const auto& srcloc = worker.GetSourceLocation(it->first);
        values[1] = worker.GetString(srcloc.file);
        values[2] = std::to_string(srcloc.line);

        const auto& zone_data = it->second;

        if (args.unwrap)
        {
            int i = 0;
            for (const auto& zone_thread_data : zone_data.zones) {
                const auto zone_event = zone_thread_data.Zone();
                const auto tId = zone_thread_data.Thread();

                const auto start = zone_event->GpuStart();
                const auto end = zone_event->GpuEnd();

                values[5] = std::to_string(start);

                auto timespan = end - start;

                values[6] = std::to_string(timespan);
                values[7] = std::to_string(tId);

                std::string row = join(values, args.separator);
                printf("%s\n", row.data());
            }
        }
    }
    return 0;
}
