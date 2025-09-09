#include <ArduinoJson.h>
#include <main.h>
#include <string>

String toJson(JsonObject &res) {
    // 建立一個 Array，把傳入的 Object 放進去
    DynamicJsonDocument response(512);
    JsonArray resArr = response.to<JsonArray>();
    resArr.add(res);

    // 序列化成字串
    String jsonOut;
    serializeJson(resArr, jsonOut);
    return jsonOut;
}