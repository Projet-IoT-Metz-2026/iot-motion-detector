# ===================================================================
# IoT Motion Detector Dashboard - Dockerfile for Railway.app
# ===================================================================

# Stage 1: Build
FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

# Copy csproj and restore dependencies
COPY backend/src/IoTDetectorDashboard/*.csproj ./
RUN dotnet restore

# Copy everything else and build
COPY backend/src/IoTDetectorDashboard/. ./
RUN dotnet publish -c Release -o /app/publish

# Stage 2: Runtime
FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS runtime
WORKDIR /app

# Install Particle CLI for Photon2 support
RUN apt-get update && \
    apt-get install -y curl && \
    curl -sL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs && \
    npm install -g particle-cli && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Copy published app
COPY --from=build /app/publish .

# Environment variables (will be overridden by Railway)
ENV ASPNETCORE_URLS=http://+:8080
ENV ASPNETCORE_ENVIRONMENT=Production

# Expose port (Railway uses PORT env variable)
EXPOSE 8080

# Start the application
ENTRYPOINT ["dotnet", "IoTDetectorDashboard.dll"]
