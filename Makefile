COMPOSE_FILE := docker-compose.yml
PROJECT_NAME := rai-jetson
DOCKER_COMPOSE ?= docker compose
COMPOSE := $(DOCKER_COMPOSE) -f $(COMPOSE_FILE) -p $(PROJECT_NAME)

.PHONY: db-up db-down db-logs db-shell db-ps db-restart

db-up:
	$(COMPOSE) up -d postgres

db-down:
	$(COMPOSE) down

db-logs:
	$(COMPOSE) logs -f --tail=200 postgres

db-shell:
	$(COMPOSE) exec postgres psql -U $${POSTGRES_USER:-robot_admin} -d $${POSTGRES_DB:-rai_website}

db-ps:
	$(COMPOSE) ps

db-restart:
	$(COMPOSE) restart postgres
