///
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "Point.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <sstream>

namespace influxdb
{

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Point::Point(const std::string& measurement) :
  mMeasurement(measurement), mTimestamp(Point::getCurrentTimestamp())
{
  mTags = {};
  mFields = {};
}

std::string Point::getTags() const {
    std::string tags;

    for (const auto &tag : mTags) {
        tags += ",";
        tags += tag.first;
        tags += "=";
        tags += tag.second;
    }

    return tags;
}

std::string Point::getFields() const {
    std::string fields;

    for (const auto &field : mFields) {
        fields += ",";
        fields += field.first;
        fields += "=";
        fields += field.second;
    }

    return fields.substr(1, fields.size());
}

Point&& Point::addField(std::string name, std::variant<int, long long int, std::string, double> value)
{
  std::stringstream convert;
  std::visit(overloaded {
    [&convert](int value) { convert << value << 'i'; },
    [&convert](long long int value) { convert << value << 'i'; },
    [&convert](double value) { convert << value; },
    [&convert](const std::string& value) { convert << '"' << value << '"'; },
    }, value);
  mFields[name] = convert.str();
  return std::move(*this);
}

Point&& Point::addTag(std::string key, std::string value)
{
  mTags[key] = value;
  return std::move(*this);
}

Point&& Point::setTimestamp(std::chrono::time_point<std::chrono::system_clock> timestamp)
{
  mTimestamp = timestamp;


  return std::move(*this);
}

auto Point::getCurrentTimestamp() -> decltype(std::chrono::system_clock::now())
{
  return std::chrono::system_clock::now();
}

std::string Point::toLineProtocol() const
{
  return mMeasurement + getTags() + " " + getFields() + " " + std::to_string(
    std::chrono::duration_cast <std::chrono::nanoseconds>(mTimestamp.time_since_epoch()).count()
  );
}

bool Point::fieldEmpty() const {
    return mFields.empty();
}

std::string Point::getName() const
{
  return mMeasurement;
}

std::chrono::time_point<std::chrono::system_clock> Point::getTimestamp() const
{
  return mTimestamp;
}

std::string Point::getTagField(const std::string &key) const
{
    auto it = mTags.find(key);
    if (it != mTags.end()) {
        return it->second;
    } else {
        it = mFields.find(key);

        if (it != mFields.end()) {
            return it->second;
        }

        return "";
    }
}


} // namespace influxdb
