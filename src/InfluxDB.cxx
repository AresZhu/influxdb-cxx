///
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "InfluxDB.h"
#include "InfluxDBException.h"

#include <iostream>
#include <memory>
#include <string>
#include <date/date.h>
#include <functional>

#ifdef INFLUXDB_WITH_BOOST

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#endif

namespace influxdb {

    InfluxDB::InfluxDB(std::unique_ptr<Transport> transport) : mBuffer(1024 * 32),
                                                               mTransport(std::move(transport)) {
        mBufferSize = 1000;
        mGlobalTags = {};
        flushInterval = 3;

        transmitThread = std::thread(std::bind(&InfluxDB::daemon, this));
    }

    void InfluxDB::daemon() {
        std::string stringBuffer{};
        std::string line;
        std::size_t size = 0;
        time_t lastSent = time(NULL);
        while (running) {
            if (mBuffer.try_dequeue(line)) {
                stringBuffer += line + "\n";
                size++;

                if (size >= mBufferSize || time(NULL) - lastSent >= flushInterval) {
                    transmit(std::move(stringBuffer));
                    size = 0;
                    stringBuffer.clear();
                    lastSent = time(nullptr);
                }
            } else {
                usleep(100);
            }
        }
    }

    void InfluxDB::batchOf(const std::size_t size) {
        mBufferSize = size;
    }

    void InfluxDB::intervalAt(const int interval) {
        flushInterval = interval;
    }

    void InfluxDB::addGlobalTag(std::string_view key, std::string_view value) {
        if (!mGlobalTags.empty()) mGlobalTags += ",";
        mGlobalTags += key;
        mGlobalTags += "=";
        mGlobalTags += value;
    }

    InfluxDB::~InfluxDB() {
        running = false;

        transmitThread.join();
    }

    void InfluxDB::transmit(std::string &&point) {
        mTransport->send(std::move(point));
    }

    void InfluxDB::write(Point &&metric) {
        mBuffer.enqueue(metric.toLineProtocol());
    }

#ifdef INFLUXDB_WITH_BOOST

    std::vector<Point> InfluxDB::query(const std::string &query) {
        auto response = mTransport->query(query);

        std::stringstream ss;
        ss << response;
        std::vector<Point> points;
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(ss, pt);

        for (auto &result : pt.get_child("results")) {
            auto isResultEmpty = result.second.find("series");
            if (isResultEmpty == result.second.not_found()) return {};
            for (auto &series : result.second.get_child("series")) {
                auto columns = series.second.get_child("columns");

                for (auto &values : series.second.get_child("values")) {
                    Point point{series.second.get<std::string>("name")};
                    auto iColumns = columns.begin();
                    auto iValues = values.second.begin();
                    for (; iColumns != columns.end() && iValues != values.second.end(); iColumns++, iValues++) {
                        auto value = iValues->second.get_value<std::string>();
                        auto column = iColumns->second.get_value<std::string>();
                        if (!column.compare("time")) {
                            std::stringstream ss(value.c_str());
                            std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> result;
                            date::from_stream(ss, "%FT%T%Z", result);
                            point.setTimestamp(result);
                            continue;
                        }

                        if (value != "null") {
                            point.addTag(column, value);
                        }
                    }
                    points.push_back(std::move(point));
                }
            }
        }
        return points;
    }

#else
    std::vector<Point> InfluxDB::query(const std::string& /*query*/)
    {
      throw InfluxDBException("InfluxDB::query", "Boost is required");
    }
#endif

} // namespace influxdb
