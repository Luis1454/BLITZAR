# Requirements Traceability

Use this workflow for any PR that changes runtime, physics, or protocol behavior.

## PR Body

- Fill the `Requirements impacted` section in `.github/PULL_REQUEST_TEMPLATE.md`.
- List one or more canonical IDs from `docs/quality/manifest/requirements.json`.
- Use one bullet per ID, for example:
  - `REQ-PROT-001`
  - `REQ-RUN-001`

## Register Update

- Update `docs/quality/traceability.csv` when requirement behavior, surface ownership, or verification references change.
- Keep the `requirement_id` column aligned with `docs/quality/manifest/requirements.json`.
- Prefer updating existing rows instead of creating aliases or duplicate IDs.

## Review Rule

- PRs that touch `runtime/`, `engine/src/physics/`, `engine/include/physics/`, or protocol integration paths must include requirement IDs.
- The PR traceability gate also requires `docs/quality/traceability.csv` to be part of the change for those PRs.
