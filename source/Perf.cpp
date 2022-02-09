//
// Created by fanick on 29.12.21.
//

#include <fstream>
#include <sstream>

#include "Perf.h"

namespace Duel6::performance {

    TraceRecorder TraceRecorder::INSTANCE;

    uint64_t now() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    uint64_t nowNano() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    void TraceRecorder::start() {
        startTs = nowNano();
        running = true;
    }

    // output is in Trace Event Format (consumable by Google Chrome)
    void TraceRecorder::save() {
        std::stringstream fileName;
        fileName << outputFileName;
        fileName << cnt++;
        fileName << ".json";
        std::ofstream os(fileName.str(), std::ios::trunc);

        os << "{\n"
           << "\"traceEvents\": [";
        for (auto &entry: data) {
            entry.output(os);
        }
        os << "],\n\"meta_user\": \"aras\",\n"
           << "\"meta_cpu_count\": \"8\"\n"
           << "}";
        os.close();
        data.clear();
    }


    TraceRecorder::entry *TraceRecorder::add(const std::string &name, double timestampUs) {
        if (!running) {
            return nullptr;
        }
        if (current == nullptr) {
            auto &result = data.emplace_back(name, (timestampUs - startTs) / 1000.0f, 0.0f);
            result.parent = current;
            current = &result;
        } else {
            auto &result = current->children.emplace_back(name, (timestampUs - startTs) / 1000.0f, 0.0f);
            result.parent = current;
            current = &result;
        }
        return current;
    }

    void TraceRecorder::add(const std::string &name, double timestampUs, double durationUs) {
        if (!running) {
            return;
        }
        data.push_back({.name = name, .start = (timestampUs - startTs) / 1000.0f, .duration = durationUs / 1000.0f});
    }

    void TraceRecorder::setCurrent(TraceRecorder::entry *current) {
        this->current = current;
    }

    TraceRecorder::~TraceRecorder() {
        if (!data.empty()) {
            save();
        }
    }

    void TraceRecorder::flip() {
        if (!running) {
            start();
        } else {
            stop();
        }
    }

    void TraceRecorder::setFileName(const std::string &fileName) {
        outputFileName = fileName;
    }

    void TraceRecorder::stop() {
        running = false;
        save();
    }

    traceEvent::traceEvent(const std::string &name) : name(name), start(nowNano()) {
        record = TraceRecorder::INSTANCE.add(name, start);
    }

    void traceEvent::end() {
        if (ended) {
            [[unlikely]]
            return;
        }
        ended = true;
        if (record != nullptr) {
            uint64_t end = nowNano();
            uint64_t dur = end - start;
            record->duration = dur / 1000.0f;
            TraceRecorder::entry *lastChild = nullptr;
            for (auto &e: record->children) {
                auto childEnd = e.start + e.duration;
                if (end < childEnd) {
                    e.duration -= childEnd - end;
                }
                if (e.start < record->start) {
                    e.start = record->start;
                }
//                if(lastChild != nullptr){
//                    if(lastChild->start + lastChild->duration >= record->start){
//                        record->start = lastChild->start + lastChild->duration + 1; //omg
//                    }
//                }
                lastChild = &e;

            }
            TraceRecorder::INSTANCE.setCurrent(record->parent);
        }
    }

    traceEvent::~traceEvent() {
        end();
    }

    tripWire::~tripWire() {
        auto res = nowNano() - t;
        if (res >= max) {
//            std::cout << name << " : " << res << "ns\n";
        }
    }

    void TraceRecorder::entry::output(std::ofstream &os) {
        os << "{ \"pid\":1, \"tid\":1, \"ts\":" << start << ", \"dur\":" << duration << ", \"ph\":\"X\", \"name\":\"" << name << "\", \"args\":{ \"start\": "
           << start << ", \"end\": " << start + duration << " } },\n";
        for (auto &entry: children) {
            entry.output(os);
        }
    }

    TraceRecorder::entry::entry(const std::string &name, double start, double duration) : name(name), start(start), duration(duration) {

    }

    perfLog::~perfLog() {
        auto res = nowNano() - t;
//            std::cout << name << " : " << res << "ns\n";
    }
}