# Tags Plugin

Content categorization and discovery.

## Overview

The tags plugin:
- Indexes posts by tags/categories
- Tracks trending content
- Manages tag-based feeds
- Calculates trending scores
- Supports content discovery

## Configuration

```ini
plugin = tags
```

No additional configuration required.

## Features

- **Tag Indexing**: Categorize posts by tags
- **Trending Calculation**: Hot/trending/promoted rankings
- **Tag Feeds**: Content organized by tag
- **Multi-tag Support**: Posts can have multiple tags
- **Author Feeds**: Posts by specific authors

## Tag System

Posts can have up to 5 tags:
- First tag is primary category
- Additional tags for discovery
- Case-insensitive matching
- Trending calculated per tag

## Trending Algorithm

Based on:
- Payout values
- Vote timing
- Content age
- Promotion (burned SBD)

## Dependencies

- `chain` - Post data

## Used By

- `tags_api` - Tag-based content queries
