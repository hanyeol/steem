# Reputation Plugin

User reputation score calculations.

## Overview

The reputation plugin:
- Calculates account reputation
- Tracks reputation changes
- Maintains reputation indices
- Supports reputation-based queries

## Configuration

```ini
plugin = reputation
```

No additional configuration required.

## Features

- **Reputation Scores**: Numerical reputation values
- **Vote-Based**: Calculated from votes received
- **Weighted**: Higher rep voters have more impact
- **Historical**: Tracks reputation over time

## Reputation Calculation

Reputation is based on:
- Net rshares received from votes
- Voter reputation weights
- Cumulative over account lifetime

Display format: Log scale centered around 25 (typical range: 0-70+)

## Dependencies

- `chain` - Vote data

## Used By

- `reputation_api` - Reputation queries
- `follow_api` - Some reputation data
