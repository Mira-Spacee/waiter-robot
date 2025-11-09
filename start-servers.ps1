# Restaurant Ordering System - Start Script
# This script starts both the frontend and backend servers

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Starting Restaurant Ordering System" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Detect current IP
Write-Host "Detecting network..." -ForegroundColor Yellow
$activeIP = (Get-NetIPAddress -AddressFamily IPv4 | Where-Object { 
    $_.IPAddress -notlike "127.*" -and 
    $_.IPAddress -notlike "169.254.*" 
} | Select-Object -First 1).IPAddress

if ($activeIP) {
    Write-Host "Network IP: $activeIP" -ForegroundColor Green
} else {
    Write-Host "WARNING: No network detected!" -ForegroundColor Red
}
Write-Host ""

# Check if servers already running
$backendRunning = Get-NetTCPConnection -LocalPort 3001 -ErrorAction SilentlyContinue
$frontendRunning = Get-NetTCPConnection -LocalPort 8080 -ErrorAction SilentlyContinue

if ($backendRunning) {
    Write-Host "Backend (3001) already running - skipping" -ForegroundColor Yellow
} else {
    Write-Host "Starting backend server (port 3001)..." -ForegroundColor Yellow
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd server; npm start"
    Start-Sleep -Seconds 2
}

if ($frontendRunning) {
    Write-Host "Frontend (8080) already running - skipping" -ForegroundColor Yellow
} else {
    Write-Host "Starting frontend server (port 8080)..." -ForegroundColor Yellow
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "npm run dev"
    Start-Sleep -Seconds 3
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host " Both servers are ready!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

if ($activeIP) {
    Write-Host "Access from any device on your network:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Customer Menu:   http://$activeIP:8080" -ForegroundColor White
    Write-Host "  Staff Login:     http://$activeIP:8080/staff/login" -ForegroundColor White
    Write-Host "  Admin Login:     http://$activeIP:8080/admin/login" -ForegroundColor White
    Write-Host ""
}

Write-Host "Local access:      http://localhost:8080" -ForegroundColor Gray
Write-Host ""
Write-Host "Press any key to close this window..." -ForegroundColor Yellow
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
