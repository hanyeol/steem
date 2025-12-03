#!groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        parallel ( "Build tests":
        {
          sh 'scripts/ci/trigger-tests.sh'
          step([$class: 'CoberturaPublisher', autoUpdateHealth: false, autoUpdateStability: false, coberturaReportFile: '**/cobertura/coverage.xml', failUnhealthy: false, failUnstable: false, maxNumberOfBuilds: 0, onlyStable: false, sourceEncoding: 'ASCII', zoomCoverageChart: false])
        },
        "Build docker image": {
          sh 'scripts/ci/trigger-build.sh'
        }, failFast: true )
      }
    }
  }
  post {
    success {
      sh 'scripts/ci/build-success.sh'
    }
    failure {
      sh 'scripts/ci/build-failure.sh'
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}