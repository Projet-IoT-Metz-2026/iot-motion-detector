# ===================================================================
# IoT Motion Detector Dashboard - Dockerfile for Railway.app
# ===================================================================

# Stage 1: Build
FROM mcr.microsoft.com/dotnet/sdk:9.0 AS build
WORKDIR /src

# Copy all project files
COPY backend/src/IoTDetectorDashboard/ ./

# Restore and build
RUN dotnet restore IoTDetectorDashboard.csproj
RUN dotnet publish IoTDetectorDashboard.csproj -c Release -o /app/publish --no-restore

# Stage 2: Runtime
FROM mcr.microsoft.com/dotnet/aspnet:9.0 AS runtime
WORKDIR /app

# Copy published app
COPY --from=build /app/publish .

# Environment variables (will be overridden by Railway)
ENV ASPNETCORE_URLS=http://+:8080
ENV ASPNETCORE_ENVIRONMENT=Production

# Expose port (Railway uses PORT env variable)
EXPOSE 8080

# Start the application
ENTRYPOINT ["dotnet", "IoTDetectorDashboard.dll"]
