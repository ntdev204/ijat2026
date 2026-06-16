COMPOSE_FILE := docker-compose.yml
PROJECT_NAME := rai-jetson
COMPOSE := docker compose -p $(PROJECT_NAME) -f $(COMPOSE_FILE)

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
