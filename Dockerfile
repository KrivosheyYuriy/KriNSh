

FROM krivosheyyuriy/terminal as previous_image

FROM alpine:latest
WORKDIR /app
COPY --from=previous_image /app/main /app/main
RUN chmod +x /app/main
RUN ls -la
CMD ["/app/main"]