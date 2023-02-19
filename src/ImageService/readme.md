ImageService
------------------------------------

a Service use to send airplane camera Image to client.
use protobuf serialization, google protobuf lib generate the serialization code.





ImageServiceHttp
------------------------------------

boost::beast::http::verb::get:

"/1"
"/2"
"/3"
"/down"
"/front"
header

```c++
response->set("X-image-height", boost::lexical_cast<std::string>(img.rows));
response->set("X-image-width", boost::lexical_cast<std::string>(img.cols));
response->set("X-image-pixel-channel", boost::lexical_cast<std::string>(img.channels()));
response->set("X-image-format", "jpg");
response->set("X-SteadyClockTimestampMs", time_string);
```

body "image/jpeg"
imageBuffer jpeg file

"/set_camera_image_size?"
"/set_camera_image_direct?"

"/time?"
OwlQueryPairsAnalyser
// "/time?setTimestamp=123456"
body "text/json"

```json5
{
  "steadyClockTimestampMs": 0,
  // "milliseconds steady_clock now time_since_epoch count"
  // setNowClockToUpdateDiffAndGetNowSteadyClock()
}
```

"/timeGet"
body "text/json"

```json5
{
  "syncClock": 0,
  // "clockTimestampMs after sync on /time? API",
  // data_r->clockTimestampMs,
  "steadyClock": 0,
  // "milliseconds steady_clock now time_since_epoch count",
  // std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count(),
  "systemClock": 0,
  // "milliseconds system_clock now time_since_epoch count",
  // std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count(),
}
```

"/downCameraId"
"/frontCameraId"
body "text/json"

```json lines
{
  "downCameraId": 1,
}
{
  "frontCameraId": 1,
}
```

"/"
body "text/html"

```C++
boost::beast::ostream(response->body())
        << "<html>\n"
        << "<head><title>Current time</title></head>\n"
        << "<body>\n"
        << "<h1>Current time</h1>\n"
        << "<p>The current time is "
        << std::time(nullptr)
        << " seconds since the epoch.</p>\n"
        << "</body>\n"
        << "</html>\n";
```

""other
body "text/plain"
"File not found\r\n"

