//
// Created by fanick on 29.12.21.
//

#ifndef DUEL6R_PERF_H
#define DUEL6R_PERF_H

#include <chrono>
//#include <iostream>
#include <sstream>
#include <list>

namespace Duel6::performance {
    uint64_t now();

    uint64_t nowNano();

    // usage:
    // 1. call start/stop on INSTANCE to generate nice trace file logging events ( see traceEvent ) between start & stop
    // 2. open the output file in the  performance tab in google chrome
    // 3. profit
    class TraceRecorder {
    public:

        struct entry {
            entry *parent = nullptr;
            std::string name;
            double start;
            double duration;
            std::list<entry> children;

            entry(const std::string &name, double start, double duration);

            void output(std::ofstream &os);
        };

        static TraceRecorder INSTANCE;

        TraceRecorder() = default;

        ~TraceRecorder();

        void flip();

        void start();

        void setFileName(const std::string &fileName);

        void stop();

        entry *add(const std::string &name, double timestampUs);

        void add(const std::string &name, double timestampUs, double durationUs);

        void setCurrent(entry *e);

    private:

        double startTs;
        std::list<TraceRecorder::entry> data;
        TraceRecorder::entry *current = nullptr;
        size_t cnt = 0; // suffix in the filename
        bool running = false;
        std::string outputFileName = "profile_";

        void save();
    };


    class traceEvent {
        std::string name;
        uint64_t start;
        bool ended = false;
        TraceRecorder::entry *record;
    public:
        traceEvent(const std::string &name);

        void end();

        ~traceEvent();
    };

    // experimental thing - could log to console
    struct perfLog {
        std::string name;
        uint64_t t = nowNano();

        perfLog(const std::string &name) : name(name) {}

        ~perfLog();
    };

    // experimental thing - could log to console when surpassing certain timeout
    struct tripWire {
        std::string name;
        uint64_t max;
        uint64_t t = nowNano();

        tripWire(const std::string &name, uint64_t max) : name(name), max(max) {}

        ~tripWire();
    };

}

#endif //DUEL6R_PERF_H
