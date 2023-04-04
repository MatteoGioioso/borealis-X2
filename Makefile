migrate:
	PGPASSWORD=postgres PGUSER=postgres migrate -source file://tests/migrations --database postgres://localhost:5001/postgres?sslmode=disable up
	PGPASSWORD=postgres PGUSER=postgres migrate -source file://tests/migrations --database postgres://localhost:5002/postgres?sslmode=disable up

up:
	docker-compose -f tests/docker-compose.yaml up --build --remove-orphans

down:
	docker-compose -f tests/docker-compose.yaml down --remove-orphans