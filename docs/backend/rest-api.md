# REST API

The C++ server uses JSON responses for metadata and records, and raw request bodies for values. The default local address is `http://127.0.0.1:8080`; deployed base URL is `https://stratadb-production.up.railway.app`.

All successful responses include `"success": true`. Errors use `{"success":false,"error":"..."}`.

## Health

`GET /health`

Returns:

```json
{"success":true,"status":"healthy","engine":"StrataDB"}
```

Example:

```bash
curl https://stratadb-production.up.railway.app/health
```

## Insert or update

`PUT /kv/:key`

The path key is URL-encoded. The request body is the exact raw value and may be text or JSON text; the engine does not parse it.

```bash
curl -X PUT "http://localhost:8080/kv/user%2F1" \
  -H "Content-Type: text/plain" \
  --data-binary '{"name":"Ada"}'
```

Success: `200 {"success":true,"message":"Value stored","key":"user/1"}`. Empty keys are rejected with 400; storage failure returns 500.

## Retrieve

`GET /kv/:key`

```bash
curl "http://localhost:8080/kv/user%2F1"
```

Success: `200 {"success":true,"key":"user/1","value":"{\"name\":\"Ada\"}"}`. Missing keys, including keys hidden by tombstones, return 404.

## Delete

`DELETE /kv/:key`

```bash
curl -X DELETE "http://localhost:8080/kv/user%2F1"
```

Success: `200 {"success":true,"message":"Key deleted"}`. A missing key returns 404. The engine records a tombstone when the delete operation is accepted.

## Inclusive range scan

`GET /scan?start=<key>&end=<key>`

Both query parameters are required and boundaries are inclusive.

```bash
curl "http://localhost:8080/scan?start=user%2F0&end=user%2F9"
```

Success:

```json
{"success":true,"count":1,"results":[{"key":"user/1","value":"..."}]}
```

Missing parameters return 400, as does `start > end`. Results are sorted by key and omit tombstones.

## List visible records

`GET /records`

```bash
curl "https://stratadb-production.up.railway.app/records"
```

Success:

```json
{
  "success": true,
  "count": 2,
  "results": [
    {"key":"a","value":"one"},
    {"key":"b","value":"two"}
  ]
}
```

The endpoint returns the complete current visible snapshot from the storage engine. It merges MemTable and SSTable records by sequence number, removes tombstones, removes duplicate keys, and emits deterministic key order. It has no server-side pagination, filtering, or query parameters. The Data Explorer performs case-insensitive key filtering and pagination in the browser after this request.

## Statistics

`GET /stats`

```bash
curl "http://localhost:8080/stats"
```

Response:

```json
{
  "success": true,
  "memtable_size": 0,
  "sstable_count": 1,
  "auto_flush_success": true
}
```

The values are current engine counters, not historical metrics.

## CORS and transport

The current server allows `*` origin, GET/PUT/DELETE/OPTIONS methods, and `Content-Type`. OPTIONS requests return 204. Production deployments should restrict origins and rely on the hosting TLS proxy; the C++ server itself does not implement authentication or TLS.
