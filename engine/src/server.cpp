
#include "../include/Database.h"

#include "httplib.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// All requests share one database instance.
// The mutex protects the current single-writer storage engine.
std::mutex databaseMutex;

// Escape strings for valid JSON output.
std::string jsonEscape(const std::string &input)
{
    std::ostringstream output;

    for (unsigned char c : input)
    {
        switch (c)
        {
        case '"':
            output << "\\\"";
            break;

        case '\\':
            output << "\\\\";
            break;

        case '\b':
            output << "\\b";
            break;

        case '\f':
            output << "\\f";
            break;

        case '\n':
            output << "\\n";
            break;

        case '\r':
            output << "\\r";
            break;

        case '\t':
            output << "\\t";
            break;

        default:
            if (c < 0x20)
            {
                const char *hex = "0123456789abcdef";

                output << "\\u00";
                output << hex[(c >> 4) & 0x0F];
                output << hex[c & 0x0F];
            }
            else
            {
                output << static_cast<char>(c);
            }
        }
    }

    return output.str();
}

void sendJson(
    httplib::Response &response,
    int status,
    const std::string &body)
{
    response.status = status;

    response.set_content(
        body,
        "application/json");
}

void sendError(
    httplib::Response &response,
    int status,
    const std::string &message)
{
    sendJson(
        response,
        status,
        "{\"success\":false,\"error\":\"" +
            jsonEscape(message) +
            "\"}");
}

int main(int argc, char *argv[])
{

    std::string dataDirectory = "./stratadb_data";
    int port = 8080;
    std::size_t flushThreshold = 1000;

    // Argument 1: Database directory
    if (argc >= 2)
    {
        dataDirectory = argv[1];
    }

    // Argument 2: Server port.
    // Prefer Railway's PORT environment variable when the
    // command line contains an unexpanded shell placeholder.

    std::string portValue;
    const char *portEnv = std::getenv("PORT");

    if (argc >= 3)
    {
        std::string argument = argv[2];

        if (argument == "$PORT" || argument == "${PORT}")
        {
            if (portEnv)
            {
                portValue = portEnv;
            }
        }
        else
        {
            portValue = argument;
        }
    }
    else if (portEnv)
    {
        portValue = portEnv;
    }

    if (!portValue.empty())
    {
        try
        {
            std::size_t parsedLength = 0;
            port = std::stoi(portValue, &parsedLength);

            if (parsedLength != portValue.size())
            {
                throw std::invalid_argument("Invalid port");
            }
        }
        catch (const std::exception &)
        {
            std::cerr << "Invalid port: " << portValue << '\n';
            return 1;
        }
    }

    // Argument 3: MemTable flush threshold
    if (argc >= 4)
    {
        try
        {
            std::size_t parsedLength = 0;

            unsigned long long parsedThreshold =
                std::stoull(argv[3], &parsedLength);

            if (
                parsedLength != std::string(argv[3]).size() ||
                parsedThreshold == 0 ||
                parsedThreshold >
                    std::numeric_limits<std::size_t>::max())
            {
                throw std::invalid_argument(
                    "Invalid flush threshold");
            }

            flushThreshold =
                static_cast<std::size_t>(parsedThreshold);
        }
        catch (const std::exception &)
        {
            std::cerr
                << "Invalid flush threshold. "
                << "It must be a positive integer.\n";
            return 1;
        }
    }

    // Initialize database with the configured flush threshold.
    Database database(dataDirectory, flushThreshold);

    if (!database.open())
    {
        std::cerr << "Failed to open StrataDB\n";
        return 1;
    }

    httplib::Server server;

    // CORS headers for the React development server.
    // Restrict the origin before exposing this server publicly.
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, PUT, DELETE, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type"}});

    // CORS preflight.
    server.Options(R"(/.*)", [](const httplib::Request &,
                                httplib::Response &response)
                   { response.status = 204; });

    // ------------------------------------------------
    // HEALTH
    // GET /health
    // ------------------------------------------------

    server.Get("/health", [](const httplib::Request &,
                             httplib::Response &response)
               { sendJson(
                     response,
                     200,
                     R"({"success":true,"status":"healthy","engine":"StrataDB"})"); });

    // ------------------------------------------------
    // PUT
    // PUT /kv/:key
    //
    // Request body = raw value.
    // ------------------------------------------------

    server.Put(
        R"(/kv/(.+))",
        [&](const httplib::Request &request,
            httplib::Response &response)
        {
            const std::string key = request.matches[1].str();
            const std::string value = request.body;

            if (key.empty())
            {
                sendError(response, 400, "Key cannot be empty");
                return;
            }

            std::lock_guard<std::mutex> lock(databaseMutex);

            if (!database.put(key, value))
            {
                sendError(response, 500, "Failed to store value");
                return;
            }

            std::string json =
                "{\"success\":true,"
                "\"message\":\"Value stored\","
                "\"key\":\"" +
                jsonEscape(key) + "\"}";

            sendJson(response, 200, json);
        });

    // ------------------------------------------------
    // GET
    // GET /kv/:key
    // ------------------------------------------------

    server.Get(
        R"(/kv/(.+))",
        [&](const httplib::Request &request,
            httplib::Response &response)
        {
            const std::string key = request.matches[1].str();

            std::lock_guard<std::mutex> lock(databaseMutex);

            std::string value;

            if (!database.get(key, value))
            {
                sendError(response, 404, "Key not found");
                return;
            }

            std::string json =
                "{\"success\":true,"
                "\"key\":\"" +
                jsonEscape(key) + "\","
                                  "\"value\":\"" +
                jsonEscape(value) + "\"}";

            sendJson(response, 200, json);
        });

    // ------------------------------------------------
    // DELETE
    // DELETE /kv/:key
    // ------------------------------------------------

    server.Delete(
        R"(/kv/(.+))",
        [&](const httplib::Request &request,
            httplib::Response &response)
        {
            const std::string key = request.matches[1].str();

            std::lock_guard<std::mutex> lock(databaseMutex);

            if (!database.remove(key))
            {
                sendError(response, 404, "Key not found");
                return;
            }

            sendJson(
                response,
                200,
                "{\"success\":true,\"message\":\"Key deleted\"}");
        });

    // ------------------------------------------------
    // SCAN
    // GET /scan?start=a&end=z
    // ------------------------------------------------

    server.Get(
        "/scan",
        [&](const httplib::Request &request,
            httplib::Response &response)
        {
            if (!request.has_param("start") ||
                !request.has_param("end"))
            {

                sendError(
                    response,
                    400,
                    "Both start and end query parameters are required");

                return;
            }

            const std::string startKey =
                request.get_param_value("start");

            const std::string endKey =
                request.get_param_value("end");

            if (startKey > endKey)
            {
                sendError(
                    response,
                    400,
                    "Start key must not be greater than end key");

                return;
            }

            std::lock_guard<std::mutex> lock(databaseMutex);

            const auto rows =
                database.scan(startKey, endKey);

            std::ostringstream json;

            json << "{\"success\":true,\"count\":"
                 << rows.size()
                 << ",\"results\":[";

            for (std::size_t i = 0; i < rows.size(); ++i)
            {

                if (i > 0)
                {
                    json << ",";
                }

                json
                    << "{"
                    << "\"key\":\""
                    << jsonEscape(rows[i].first)
                    << "\","
                    << "\"value\":\""
                    << jsonEscape(rows[i].second)
                    << "\""
                    << "}";
            }

            json << "]}";

            sendJson(response, 200, json.str());
        });

    // ------------------------------------------------
    // LIST
    // GET /records
    //
    // Returns all currently visible records in key order.
    // Pagination is intentionally handled by clients because
    // this endpoint is a complete database snapshot.
    // ------------------------------------------------

    server.Get(
        "/records",
        [&](const httplib::Request &,
            httplib::Response &response)
        {
            std::lock_guard<std::mutex> lock(databaseMutex);
            const auto rows = database.list();
            std::ostringstream json;

            json << "{\"success\":true,\"count\":"
                 << rows.size()
                 << ",\"results\":[";

            for (std::size_t i = 0; i < rows.size(); ++i)
            {
                if (i > 0)
                {
                    json << ",";
                }

                json << "{\"key\":\""
                     << jsonEscape(rows[i].first)
                     << "\",\"value\":\""
                     << jsonEscape(rows[i].second)
                     << "\"}";
            }

            json << "]}";
            sendJson(response, 200, json.str());
        });

    // ------------------------------------------------
    // STATS
    // GET /stats
    // ------------------------------------------------

    server.Get(
        "/stats",
        [&](const httplib::Request &,
            httplib::Response &response)
        {
            std::lock_guard<std::mutex> lock(databaseMutex);

            std::ostringstream json;

            json
                << "{"
                << "\"success\":true,"
                << "\"memtable_size\":"
                << database.memTableSize()
                << ","
                << "\"sstable_count\":"
                << database.sstableCount()
                << ","
                << "\"auto_flush_success\":"
                << (database.wasLastAutoFlushSuccessful()
                        ? "true"
                        : "false")
                << "}";

            sendJson(response, 200, json.str());
        });

    // ------------------------------------------------
    // SERVER STARTUP
    // ------------------------------------------------

    std::cout << "=================================\n";
    std::cout << "       StrataDB HTTP Server\n";
    std::cout << "=================================\n";
    std::cout << "Data directory: " << dataDirectory << '\n';
    std::cout << "Port: " << port << '\n';
    std::cout << "Flush threshold: " << flushThreshold << '\n';

    // Use Railway's HOST when deployed.
    // Otherwise, default to localhost for local development.
    const char *hostEnv = std::getenv("HOST");
    const std::string host =
        (hostEnv && *hostEnv) ? hostEnv : "127.0.0.1";

    std::cout << "Health: http://" << host
              << ":" << port << "/health\n";
    std::cout << "=================================\n";

    if (!server.listen(host, port))
    {
        std::cerr << "Failed to start HTTP server\n";
        return 1;
    }

    return 0;
}