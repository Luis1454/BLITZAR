# Decision 038: Directive Configuration Format

Status: accepted
Issue: #635
Plan version: 1.0.22

## Context

The clean-room rewrite needs to accept configuration files written in the
directive syntax used by the previous product format, without importing the
previous implementation or its feature semantics. A syntax-only boundary is
required so configuration compatibility can be tested independently from
simulation behavior.

## Decision

Implement an internal, bounded parser for one directive per line:

- comments begin with `#` and may follow a directive;
- a directive is `name(key=value, ...)`;
- names use identifier characters, while values may be quoted or unquoted;
- quoted values preserve empty strings and decode only basic escaped characters;
- repeated directives and argument order are preserved;
- malformed input, excessive input, and trailing arguments are rejected without
  mutating the destination configuration.

The parser stores generic directive data only. It does not generate particles,
select solvers, interpret scenes, or expose a compatibility implementation. Any
semantic application to `Sim` requires a separate contract and qualification.
The public C and C++ SDK surfaces remain unchanged.

## Consequences

- Existing directive-shaped files can be validated by `blitzar_config_test`.
- Configuration loading allocations are confined to the initialization-time
  parser and do not alter the simulation steady-state contract.
- Syntax compatibility is proven separately from semantic feature support.
- Unsupported legacy directives remain data rather than silently acquiring old
  behavior.
