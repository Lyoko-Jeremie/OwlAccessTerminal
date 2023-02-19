CommandService
------------------------------------

a Service use to receive airplane play command. 
the command are short , and packed into UDP package. 
use JSON serialization, Boost.JSON lib. 




CmdServiceHttpConnect
------------------------------------

boost::beast::http::verb::post:



/cmd
request_.body() jsonS
OwlProcessJsonMessage::process_json_message

/tagInfo
request_.body() jsonS
```json5
{
  "imageX": 0,
  "imageY": 0,
  "tagList": [
    {
      id: 0,
      ham: 0,
      dm: 0.0,
      cX: 0.0,
      cY: 0.0,
      cLTx: 0.0,
      cLTy: 0.0,
      cRTx: 0.0,
      cRTy: 0.0,
      cRBx: 0.0,
      cRBy: 0.0,
      cLBx: 0.0,
      cLBy: 0.0,
    },
    // ...
  ],
  "centerTag": {
    id: 0,
    ham: 0,
    dm: 0.0,
    cX: 0.0,
    cY: 0.0,
    cLTx: 0.0,
    cLTy: 0.0,
    cRTx: 0.0,
    cRTy: 0.0,
    cRBx: 0.0,
    cRBy: 0.0,
    cLBx: 0.0,
    cLBy: 0.0,
  },
}
```


------------------------------------

boost::beast::http::verb::get:



/nowTimestamp
body "text/html"
`milliseconds steady_clock time_since_epoch count`

/AirplaneState
body "application/json"
```json5
// failed
{
  "msg": "newestAirplaneState nullptr",
  "error": "nullptr",
  "result": false
}
```
```json5
// ok
{
  "result": true,
  "state": {
    "timestamp": 0,
    // state->timestamp,
    "stateFly": 0,
    // state->stateFly,
    "pitch": 0,
    // state->pitch,
    "roll": 0,
    // state->roll,
    "yaw": 0,
    // state->yaw,
    "vx": 0,
    // state->vx,
    "vy": 0,
    // state->vy,
    "vz": 0,
    // state->vz,
    "high": 0,
    // state->high,
    "voltage": 0,
    // state->voltage,
  },
  "nowTimestamp": 0,
  // milliseconds steady_clock now time_since_epoch count
  "nowTimestampC": 0,
  // milliseconds system_clock now time_since_epoch count
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



