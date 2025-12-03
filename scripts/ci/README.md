# CI Scripts

Continuous Integration scripts for Jenkins build automation.

## Overview

Shell scripts for automated Docker builds, tests, and GitHub status updates in Jenkins CI environment.

## Scripts

| Script | Purpose |
|--------|---------|
| **[build-release.sh](build-release.sh)** | Build and push Docker image |
| **[build-tests.sh](build-tests.sh)** | Run tests and generate coverage reports |
| **[trigger-build.sh](trigger-build.sh)** | Main build orchestration script |
| **[trigger-tests.sh](trigger-tests.sh)** | Main test orchestration script |
| **[build-pending.sh](build-pending.sh)** | Update GitHub status to "pending" |
| **[build-success.sh](build-success.sh)** | Update GitHub status to "success" |
| **[build-failure.sh](build-failure.sh)** | Update GitHub status to "failure" |

## Workflow

### Build Workflow

```bash
trigger-build.sh
  ├── build-pending.sh      # Set GitHub status: pending
  ├── build-release.sh      # Build & push Docker image
  ├── build-success.sh      # Set GitHub status: success (if success)
  └── build-failure.sh      # Set GitHub status: failure (if failure)
```

### Test Workflow

```bash
trigger-tests.sh
  ├── build-pending.sh      # Set GitHub status: pending
  ├── build-tests.sh        # Run tests & extract coverage
  ├── build-success.sh      # Set GitHub status: success (if success)
  └── build-failure.sh      # Set GitHub status: failure (if failure)
```

## build-release.sh

Build and push Docker image to Docker Hub.

### Usage

```bash
export BRANCH_NAME=main
export DOCKER_USER=<username>
export DOCKER_PASS=<password>
./build-release.sh
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `BRANCH_NAME` | Git branch name (determines Docker tag) |
| `DOCKER_USER` | Docker Hub username |
| `DOCKER_PASS` | Docker Hub password |

### Behavior

- **Branch `stable`**: Tags as `steemit/steem:latest`
- **Other branches**: Tags as `steemit/steem:$BRANCH_NAME`
- Builds with `BUILD_STEP=2` argument
- Pushes to Docker Hub
- Cleans up exited containers

### Example

```bash
# Build from stable branch
export BRANCH_NAME=stable
./build-release.sh
# → steemit/steem:latest

# Build from feature branch
export BRANCH_NAME=feature-xyz
./build-release.sh
# → steemit/steem:feature-xyz
```

## build-tests.sh

Run tests in Docker and extract code coverage reports.

### Usage

```bash
export WORKSPACE=/path/to/workspace
./build-tests.sh
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `WORKSPACE` | Jenkins workspace directory |

### Behavior

- Builds test image with `BUILD_STEP=1`
- Runs tests in container
- Extracts Cobertura coverage reports to `$WORKSPACE/cobertura/`
- Cleans up exited containers

### Output

Coverage reports copied to:
```
$WORKSPACE/cobertura/
```

## GitHub Status Updates

Scripts to update GitHub commit status via API.

### build-pending.sh

```bash
export GITHUB_SECRET=<token>
export BUILD_URL=<jenkins_url>
./build-pending.sh
```

Updates commit status to **pending**.

### build-success.sh

```bash
export GITHUB_SECRET=<token>
export BUILD_URL=<jenkins_url>
./build-success.sh
```

Updates commit status to **success**.

### build-failure.sh

```bash
export GITHUB_SECRET=<token>
export BUILD_URL=<jenkins_url>
./build-failure.sh
```

Updates commit status to **failure**.

### Environment Variables

| Variable | Description |
|----------|-------------|
| `GITHUB_SECRET` | GitHub personal access token |
| `BUILD_URL` | Jenkins build URL |

### GitHub API Call

```bash
curl -XPOST \
  -H "Authorization: token $GITHUB_SECRET" \
  https://api.github.com/repos/steemit/steem/statuses/$(git rev-parse HEAD) \
  -d '{
    "state": "pending|success|failure",
    "target_url": "jenkins_url",
    "description": "...",
    "context": "jenkins-ci-steemit"
  }'
```

## trigger-build.sh

Main build orchestration script.

### Usage

```bash
export WORKSPACE=/path/to/workspace
export BRANCH_NAME=master
export DOCKER_USER=<username>
export DOCKER_PASS=<password>
export GITHUB_SECRET=<token>
export BUILD_URL=<jenkins_url>
./trigger-build.sh
```

### Flow

1. Set GitHub status to "pending"
2. Run build script
3. If success: Update status to "success"
4. If failure: Update status to "failure" and exit 1

### Jenkins Integration

```groovy
// Jenkinsfile
stage('Build') {
    steps {
        sh './scripts/ci/trigger-build.sh'
    }
}
```

## trigger-tests.sh

Main test orchestration script.

### Usage

```bash
export WORKSPACE=/path/to/workspace
export GITHUB_SECRET=<token>
export BUILD_URL=<jenkins_url>
./trigger-tests.sh
```

### Flow

1. Set GitHub status to "pending"
2. Run test script
3. Extract coverage reports
4. If success: Update status to "success"
5. If failure: Update status to "failure" and exit 1

### Jenkins Integration

```groovy
// Jenkinsfile
stage('Test') {
    steps {
        sh './scripts/ci/trigger-tests.sh'
    }
    post {
        always {
            publishHTML([
                reportDir: 'cobertura',
                reportFiles: 'index.html',
                reportName: 'Coverage Report'
            ])
        }
    }
}
```

## Docker Build Steps

### BUILD_STEP=1 (Tests)

```dockerfile
# In Dockerfile
ARG BUILD_STEP
RUN if [ "$BUILD_STEP" = "1" ]; then \
    make test && \
    generate_coverage_reports; \
    fi
```

### BUILD_STEP=2 (Release)

```dockerfile
ARG BUILD_STEP
RUN if [ "$BUILD_STEP" = "2" ]; then \
    make release_build; \
    fi
```

## Jenkins Pipeline Example

```groovy
pipeline {
    agent any

    environment {
        DOCKER_USER = credentials('docker-user')
        DOCKER_PASS = credentials('docker-pass')
        GITHUB_SECRET = credentials('github-token')
    }

    stages {
        stage('Test') {
            steps {
                sh './scripts/ci/trigger-tests.sh'
            }
        }

        stage('Build') {
            when {
                branch 'master'
            }
            steps {
                sh './scripts/ci/trigger-build.sh'
            }
        }
    }

    post {
        success {
            sh './scripts/ci/build-success.sh'
        }
        failure {
            sh './scripts/ci/build-failure.sh'
        }
    }
}
```

## Error Handling

All scripts use `set -e` to exit immediately on error:

```bash
#!/bin/bash
set -e  # Exit on any error
```

## Docker Cleanup

Scripts automatically clean up exited containers:

```bash
sudo docker rm -v $(docker ps -a -q -f status=exited) || true
```

The `|| true` ensures cleanup failures don't fail the build.

## Security Notes

### Credentials Management

- Never commit credentials to repository
- Use Jenkins credentials plugin
- Pass via environment variables

### GitHub Token Permissions

Required scopes for `GITHUB_SECRET`:
- `repo:status` - Update commit status

### Docker Hub Authentication

```bash
# Login is automated in buildscript.sh
docker login --username=$DOCKER_USER --password=$DOCKER_PASS
```

## Troubleshooting

### Build Failures

```bash
# Check Docker build logs
docker logs <container_id>

# Manual build test
docker build --build-arg BUILD_STEP=2 -t test .
```

### GitHub Status Not Updating

```bash
# Verify token has correct permissions
curl -H "Authorization: token $GITHUB_SECRET" \
  https://api.github.com/repos/steemit/steem

# Check commit hash
git rev-parse HEAD
```

### Coverage Reports Not Generated

```bash
# Verify WORKSPACE is set
echo $WORKSPACE

# Check container volume mount
docker run -v $WORKSPACE:/var/jenkins steemit/steem:tests ls -la /var/cobertura
```

### Docker Permission Issues

```bash
# Add user to docker group
sudo usermod -aG docker $USER

# Or use sudo (as scripts do)
sudo docker build ...
```

## Local Testing

### Test Build Script

```bash
export BRANCH_NAME=test
export DOCKER_USER=testuser
export DOCKER_PASS=testpass
./build-release.sh
```

### Test Coverage Generation

```bash
export WORKSPACE=/tmp/test-workspace
mkdir -p $WORKSPACE
./build-tests.sh
ls -la $WORKSPACE/cobertura/
```

### Skip GitHub Updates

Comment out status update lines:

```bash
# ./build-pending.sh  # Skip this in local testing
./build-release.sh
# ./build-success.sh  # Skip this in local testing
```

## Additional Resources

- [Dockerfile](../Dockerfile) - Multi-stage build configuration
- [Jenkins Documentation](https://www.jenkins.io/doc/)
- [GitHub Status API](https://docs.github.com/en/rest/commits/statuses)
- [Docker Documentation](https://docs.docker.com/)

## License

See [LICENSE](../LICENSE.md)
