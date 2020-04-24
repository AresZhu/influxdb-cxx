///
/// \author Adam Wegrzynek
///

#ifndef INFLUXDATA_POINT_H
#define INFLUXDATA_POINT_H

#include <string>
#include <chrono>
#include <variant>
#include <unordered_map>

namespace influxdb
{

/// \brief Represents a point
class Point
{
  public:
    /// Constructs point based on measurement name
    Point(const std::string& measurement);

    /// Default destructor
    ~Point() = default;

    /// Adds a tags
    Point&& addTag(std::string key, std::string value);

    /// Adds filed
    Point&& addField(std::string name, std::variant<int, long long int, std::string, double> value);

    /// Generetes current timestamp
    static auto getCurrentTimestamp() -> decltype(std::chrono::system_clock::now());

    /// Converts point to Influx Line Protocol
    std::string toLineProtocol() const;

    /// Sets custom timestamp
    Point&& setTimestamp(std::chrono::time_point<std::chrono::system_clock> timestamp);

    /// Name getter
    std::string getName() const;

    /// Timestamp getter
    std::chrono::time_point<std::chrono::system_clock> getTimestamp() const;

    std::string getTagField(const std::string &key) const;

    bool fieldEmpty() const;

private:
    std::string getTags() const;
    std::string getFields() const;

  protected:
    /// A name
    std::string mMeasurement;

    /// A timestamp
    std::chrono::time_point<std::chrono::system_clock> mTimestamp;

    /// Tags
    std::unordered_map<std::string, std::string> mTags;

    /// Fields
    std::unordered_map<std::string, std::string> mFields;
};

} // namespace influxdb

#endif // INFLUXDATA_POINT_H
