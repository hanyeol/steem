# Voting Threshold Reward Reduction

## Overview

The Voting Threshold Reward Reduction mechanism is designed to prevent excessive rewards in the early stages of a blockchain network when there are few active users. This feature ensures that content rewards are proportionally reduced when the total voting power across the network falls below a specified threshold, with the reduced portion being added to the vesting pool for fair distribution among all stakeholders.

## Motivation

In the early stages of a blockchain network launch, a small number of users with limited voting power could potentially claim disproportionately large rewards from the reward pool. This creates several problems:

1. **Unfair reward distribution** - Small voting activity receives full rewards designed for a mature network
2. **Economic imbalance** - Early adopters can extract excessive value with minimal stake participation
3. **Network health** - Rewards should reflect actual network engagement and usage

This mechanism addresses these issues by scaling rewards proportionally to actual network participation.

## Mechanism

### Calculation Process

On every block, the following calculation is performed:

1. **Calculate Total Voting Power**
   - Sum up all rshares from posts/comments receiving rewards in the current block's reward pool

2. **Calculate Network Participation Ratio**
   ```
   participation_ratio = total_voting_rshares / (total_network_vests × percent_network_voting_threshold)
   ```

   Where:
   - `total_voting_rshares` = Sum of all voting weight (rshares) for content being rewarded
   - `total_network_vests` = Total VESTS in the entire network
   - `percent_network_voting_threshold` = Required participation threshold (chain parameter)

3. **Apply Reduction Factor**
   ```
   if participation_ratio >= 1.0:
       reduction_factor = 1.0  // No reduction, full rewards
   else:
       reduction_factor = participation_ratio  // Proportional reduction
   ```

4. **Distribute Rewards**
   ```
   actual_content_reward = original_reward × reduction_factor
   vesting_pool_bonus = original_reward × (1 - reduction_factor)
   ```

### Example Scenarios

#### Scenario 1: Genesis Launch - Below Threshold
```
Total network VESTS: 1,000,000
Threshold rate: 100% (1.0)
Required voting power: 1,000,000 VESTS (100% of network)
Actual voting power: 300,000 VESTS (only 30% participating)

participation_ratio = 300,000 / 1,000,000 = 0.3
reduction_factor = 0.3

Original content reward: 100 STEEM
Actual content reward: 30 STEEM (30%)
Vesting pool bonus: 70 STEEM (70%)

Result: Content receives 30% of rewards, vesting pool receives 70%
```

#### Scenario 2: Early Growth - Gradually Relaxed
```
Total network VESTS: 100,000,000
Threshold rate: 10% (0.1)
Required voting power: 10,000,000 VESTS
Actual voting power: 15,000,000 VESTS

participation_ratio = 15,000,000 / 10,000,000 = 1.5
reduction_factor = 1.0 (capped at 1.0)

Result: Content receives 100% of rewards, no vesting pool bonus
```

#### Scenario 3: Mature Network - Minimal Threshold
```
Total network VESTS: 10,000,000,000
Threshold rate: 0.5% (0.005)
Required voting power: 50,000,000 VESTS
Actual voting power: 80,000,000 VESTS

participation_ratio = 80,000,000 / 50,000,000 = 1.6
reduction_factor = 1.0 (capped at 1.0)

Result: Content receives 100% of rewards, no vesting pool bonus
```

## Implementation Details

### Chain Parameters

A new chain parameter is introduced to control the threshold:

- **`percent_network_voting_threshold`** - Percentage of total network VESTS that must participate in voting for full rewards
  - Type: `uint16_t` (basis points, e.g., 10000 = 100%)
  - Default: 10000 (100% for genesis launch)
  - Modifiable via: Hardfork or witness consensus
  - Note: Should be gradually reduced as network matures

### Code Locations

The implementation involves modifications to several core components:

1. **Protocol Layer** (`libraries/protocol/`)
   - Add chain parameter definition
   - Update reward calculation structures

2. **Chain Layer** (`libraries/chain/database.cpp`)
   - Modify reward distribution logic in `process_funds()`
   - Calculate total voting power and participation ratio
   - Apply reduction factor to content rewards
   - Add reduced portion to vesting pool

### Database Objects

The following objects are involved:

- **`dynamic_global_property_object`** - Tracks total VESTS and vesting pool balance
- **`comment_object`** - Contains rshares for each post/comment
- **`reward_fund_object`** - Manages reward pool distribution

## Economic Impact

### Benefits

1. **Protects early-stage economics** - Prevents reward exploitation when network is small
2. **Fair distribution to all stakeholders** - Reduced rewards go to vesting pool, benefiting all VESTS holders proportionally
3. **Encourages organic growth** - Rewards scale naturally with actual network usage
4. **Incentivizes voting participation** - Stakeholders are motivated to actively participate in content curation
5. **Long-term alignment** - Vesting pool bonus encourages long-term holding and network commitment

### Trade-offs

1. **Reduced early author rewards** - Content creators receive less during low-activity periods
2. **Complexity** - Adds computational overhead to reward calculations
3. **Parameter sensitivity** - Threshold setting requires careful calibration

## Configuration

### For Node Operators

No special configuration is required. The mechanism is automatically enforced by consensus rules once activated via hardfork.

### For Witnesses

Witnesses can propose changes to the threshold parameter through the standard chain parameter update process.

Recommended threshold values based on network stage:
- **Genesis launch**: 100% of total VESTS (all stakes must participate in voting initially)
- **Bootstrap phase**: Linearly decrease from 100% to 1% as supply grows to bootstrap threshold
- **Early growth**: 1% - 10% of total VESTS (gradually relax as network grows)
- **Mature network**: 0.1% - 1% of total VESTS, or consider disabling (set to 0)

## Activation

This feature will be active from genesis launch. The initial configuration includes:

1. Threshold parameter is set in genesis block configuration
2. Default threshold rate: 10000 (100% - requiring full network participation initially)
3. Threshold should be gradually reduced via witness consensus as the network grows

## Monitoring

Node operators and witnesses should monitor:

1. **Current participation ratio** - Via API or logs
2. **Reduction factor applied** - Track how often rewards are reduced
3. **Vesting pool bonus accumulation** - Additional rewards added to vesting pool

Suggested metrics to expose via API:
```json
{
  "voting_threshold_info": {
    "total_network_vests": "1000000000.000000 VESTS",
    "total_voting_rshares": "500000000000",
    "percent_network_voting_threshold": 10000,
    "participation_ratio": 0.5,
    "current_reduction_factor": 0.5,
    "blocks_below_threshold": 1234,
    "total_rewards_to_vesting_pool": "50000.000 STEEM"
  }
}
```

## Testing

### Unit Tests

Required test coverage:
- Reward calculation with various participation ratios
- Threshold boundary conditions (exactly at threshold, just above/below)
- Vesting pool bonus distribution
- Integration with existing reward fund logic

### Integration Tests

- Full block processing with reward reduction
- Multiple posts/comments in single block
- Transition across threshold boundary
- Hardfork activation

## Future Considerations

1. **Dynamic threshold adjustment** - Automatically adjust based on network growth metrics
2. **Graduated reduction curve** - Non-linear scaling for smoother transitions
3. **Per-tag thresholds** - Different requirements for different content categories
4. **Time-based decay** - Gradually phase out mechanism as network matures

## References

- [Reward Distribution Algorithm](../technical-reference/reward-distribution.md)
- [Witness Scheduling](../technical-reference/witness-scheduling.md)
- [Chain Parameters](../technical-reference/chain-parameters.md)

## Version History

- **Version 1.0** - Initial specification (2025-11-08)
