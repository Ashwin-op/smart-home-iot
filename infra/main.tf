locals {
  context = {
    namespace = "iot"
    stage     = "dev"
    name      = "fastapi"
  }
  region = "us-east-1"
}

data "aws_caller_identity" "current" {}

module "ecr" {
  source = "cloudposse/ecr/aws"

  image_tag_mutability = "MUTABLE"

  namespace = local.context.namespace
  stage     = local.context.stage
  name      = local.context.name
}

resource "time_sleep" "wait_role_creation" {
  depends_on = [aws_iam_role.service_role]

  create_duration = "15s"
}

resource "aws_iam_role" "service_role" {
  name = "${local.context.name}-service-role"

  managed_policy_arns = [
    "arn:aws:iam::aws:policy/service-role/AWSAppRunnerServicePolicyForECRAccess",
  ]

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = "sts:AssumeRole"
        Effect = "Allow"
        Principal = {
          Service = [
            "build.apprunner.amazonaws.com",
            "tasks.apprunner.amazonaws.com"
          ]
        }
      },
    ]
  })

  inline_policy {
    name = "dynamodb-access"
    policy = jsonencode({
      Version = "2012-10-17"
      Statement = [
        {
          Action = [
            "dynamodb:GetItem",
            "dynamodb:PutItem",
            "dynamodb:Query",
            "dynamodb:Scan",
            "dynamodb:UpdateItem",
            "dynamodb:DeleteItem"
          ]
          Effect   = "Allow"
          Resource = "arn:aws:dynamodb:${local.region}:${data.aws_caller_identity.current.account_id}:table/${local.context.name}-table"
        }
      ]
    })
  }
}


resource "aws_dynamodb_table" "table" {
  name           = "${local.context.name}-table"
  read_capacity  = 5
  write_capacity = 5
  attribute {
    name = "device"
    type = "S"
  }
  hash_key = "device"
}

resource "aws_apprunner_service" "service" {
  service_name = local.context.name

  source_configuration {
    authentication_configuration {
      access_role_arn = aws_iam_role.service_role.arn
    }
    image_repository {
      image_configuration {
        port = 8000
      }
      image_identifier      = "${module.ecr.repository_url}:latest"
      image_repository_type = "ECR"
    }
    auto_deployments_enabled = true
  }

  instance_configuration {
    instance_role_arn = aws_iam_role.service_role.arn
  }

  depends_on = [time_sleep.wait_role_creation]
}


output "service_url" {
  value = aws_apprunner_service.service.service_url
}
