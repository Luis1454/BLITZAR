# Web Gateway Contract

Canonical compatibility governance for this surface is defined in `docs/quality/interface-contracts.md` (`CTR-WEB-001`).

## Purpose

- The web gateway is an optional adapter for browser clients.
- It does not change compute-core behavior.
- It translates HTTP and WebSocket traffic to the existing `server-json-v1` TCP backend protocol.

## Transport Surfaces

- HTTP base path: `/api/v1`
- WebSocket endpoint: `/ws`
- Health endpoint: `/healthz`

## HTTP Endpoints

- `GET /healthz`
  - returns gateway liveness plus the backend schema label
- `GET /api/v1/schema`
  - returns the machine-readable `server-json-v1` schema artifact mirrored from `rust/blitzar-protocol`
- `GET /api/v1/status`
  - proxies backend `status`
- `GET /api/v1/snapshot?max_points=<uint32>`
  - proxies backend `get_snapshot`
- `POST /api/v1/command`
  - request body is the same JSON envelope as backend TCP requests
  - response body is the raw backend response envelope
- `POST /api/v1/load`
  - request body: `{"path":"...","format":"auto|..."}`
  - translates to backend `load`
- `POST /api/v1/export`
  - request body: `{"path":"...","format":"..."}` with both fields optional
  - translates to backend `export`

## WebSocket Messages

- Client -> gateway
  - text frames only
  - payload is the same JSON command envelope used by `POST /api/v1/command`
- Gateway -> client
  - `{"type":"status","payload":<StatusPayload>}`
  - `{"type":"snapshot","payload":<SnapshotPayload>}`
  - `{"type":"command_response","payload":<backend response envelope>}`
  - `{"type":"error","message":"<text>"}`

## Compatibility Rules

- The gateway is compatible within `web-gateway-v1` when:
  - existing HTTP routes keep their semantics;
  - WebSocket event type names stay unchanged;
  - backend command names and payload shapes remain pass-through compatible with `server-json-v1`.
- The following are breaking:
  - renaming or removing existing routes;
  - renaming or removing existing WebSocket event types;
  - rewriting backend command names or mandatory payload fields in transit.

## Deployment Notes

- The gateway is built from the Rust workspace as `blitzar-web-gateway`.
- It is intended for loopback or explicitly managed development deployments.
- Backend auth, when configured on `blitzar-server`, is forwarded through the gateway using the configured server token.

