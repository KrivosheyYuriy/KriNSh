# stage 1
FROM gcc:latest as builder
WORKDIR /app
COPY app/main.c .
RUN gcc -o main main.c -static
RUN chmod +x /app/main
# static значит, что все библиотеки включены

# stage 2
FROM scratch
WORKDIR /app
COPY --from=builder /app/main /app/main
