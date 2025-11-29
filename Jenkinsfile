#!groovy
pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        parallel ( "Build tests":
        {
          sh 'scripts/ci/triggertests.sh'
          step([$class: 'CoberturaPublisher', autoUpdateHealth: false, autoUpdateStability: false, coberturaReportFile: '**/cobertura/coverage.xml', failUnhealthy: false, failUnstable: false, maxNumberOfBuilds: 0, onlyStable: false, sourceEncoding: 'ASCII', zoomCoverageChart: false])
        },
        "Build docker image": {
          sh 'scripts/ci/triggerbuild.sh'
        }, failFast: true )
      }
    }
  }
  post {
    success {
      sh 'scripts/ci/buildsuccess.sh'
    }
    failure {
      sh 'scripts/ci/buildfailure.sh'
      slackSend (color: '#ff0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
    }
  }
}