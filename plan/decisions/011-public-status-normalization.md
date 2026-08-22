# Decision 011: Public Status Normalization

Status: accepted
Plan version: 1.0.6

## Decision

The C++ SDK converts every `blitzar_status` through `blitzar::FromCStatus`.
The function explicitly maps each ABI value to its corresponding scoped C++
enum value. Values unknown to the current ABI normalize to
`blitzar::Status::InternalError` and never cross the boundary through an
unchecked enum cast.

The C ABI remains the source representation for exported functions. The C++
facade owns the only conversion into `blitzar::Status`, and all constructors
and operation-result updates use that conversion.

## Consequences

- Future internal status values cannot silently create an invalid C++ enum.
- C and C++ callers share the same known-value mapping.
- The contract test covers every public status and an injected unknown value.
