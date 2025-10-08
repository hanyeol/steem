# Schema Library

Schema generation and documentation for Steem types and operations.

## Overview

The `schema` library provides:
- Type introspection
- Schema generation
- API documentation support
- Type metadata

## Purpose

Generates schema information for:
- Client library development
- API documentation
- Type validation
- Code generation in other languages

## Usage

Used by:
- Documentation generation tools
- Client library generators
- Type validators

## Build

```bash
cd build
make steem_schema
```

## Dependencies

- `steem_protocol` - Type definitions
- `fc` - Reflection utilities
