using System;
using System.Collections.Generic;
using System.Text.Json;
using Microsoft.Azure.Functions.Worker;
using Microsoft.Azure.Functions.Worker.Http;
using Microsoft.Extensions.Logging;

namespace IoTHubProcessor
{
    public class ApiHandlers
    {
        private readonly ILogger<ApiHandlers> _logger;

        public ApiHandlers(ILogger<ApiHandlers> logger)
        {
            _logger = logger;
        }

        [Function("GetDevices")]
        public HttpResponseData GetDevices([HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "devices")] HttpRequestData req)
        {
            var devices = new[] {
                new { id = "esp32-001", name = "ESP32-Sensor-01", type = "ESP32", status = "active", lastSignal = "Il y a 2 min", deviceId = "esp32-dev-001-azure" },
                new { id = "photon2-001", name = "Photon2-Hub-01", type = "Photon2", status = "active", lastSignal = "Il y a 5 min", deviceId = "photon2-hub-001-azure" },
                new { id = "esp32-002", name = "ESP32-Sensor-02", type = "ESP32", status = "inactive", lastSignal = "Il y a 2 h", deviceId = "esp32-dev-002-azure" }
            };

            var response = req.CreateResponse();
            response.Headers.Add("Content-Type", "application/json; charset=utf-8");
            response.Headers.Add("Access-Control-Allow-Origin", "*");
            response.WriteString(JsonSerializer.Serialize(devices));
            return response;
        }

        [Function("GetEvents")]
        public HttpResponseData GetEvents([HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "events")] HttpRequestData req)
        {
            var events = new[] {
                new { timestamp = "2025-10-27 14:32:15", message = "Détection PIR - ESP32-001", level = "info" },
                new { timestamp = "2025-10-27 14:28:42", message = "Température élevée - ESP32-003", level = "warning" },
                new { timestamp = "2025-10-27 14:15:08", message = "Connexion réussie - Photon2-001", level = "info" }
            };

            var response = req.CreateResponse();
            response.Headers.Add("Content-Type", "application/json; charset=utf-8");
            response.Headers.Add("Access-Control-Allow-Origin", "*");
            response.WriteString(JsonSerializer.Serialize(events));
            return response;
        }

        [Function("GetMetrics")]
        public HttpResponseData GetMetrics([HttpTrigger(AuthorizationLevel.Anonymous, "get", Route = "metrics")] HttpRequestData req)
        {
            var metrics = new {
                activityData = new[] {
                    new { time = "00:00", value = 45 }, new { time = "04:00", value = 32 }, new { time = "08:00", value = 68 },
                    new { time = "12:00", value = 85 }, new { time = "16:00", value = 72 }, new { time = "20:00", value = 55 }
                },
                temperatureData = new[] {
                    new { time = "00:00", temp = 22.5, humidity = 45 }, new { time = "04:00", temp = 21.8, humidity = 48 },
                    new { time = "08:00", temp = 23.2, humidity = 42 }, new { time = "12:00", temp = 25.1, humidity = 38 },
                    new { time = "16:00", temp = 24.5, humidity = 40 }, new { time = "20:00", temp = 23.0, humidity = 44 }
                }
            };

            var response = req.CreateResponse();
            response.Headers.Add("Content-Type", "application/json; charset=utf-8");
            response.Headers.Add("Access-Control-Allow-Origin", "*");
            response.WriteString(JsonSerializer.Serialize(metrics));
            return response;
        }
    }
}
