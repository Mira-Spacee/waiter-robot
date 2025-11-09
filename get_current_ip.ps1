# Find your current IP address for hotspot or WiFi
# Run this to see what IP to use

Write-Host ""
Write-Host "Detecting your network configuration..." -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Gray
Write-Host ""

$adapters = Get-NetIPAddress -AddressFamily IPv4 | Where-Object { 
    $_.IPAddress -notlike "127.*" -and 
    $_.IPAddress -notlike "169.254.*" 
}

$found = $false

foreach ($adapter in $adapters) {
    $interface = Get-NetAdapter -InterfaceIndex $adapter.InterfaceIndex
    $status = $interface.Status
    
    if ($status -eq "Up") {
        $found = $true
        Write-Host "ACTIVE CONNECTION FOUND" -ForegroundColor Green
        Write-Host "   Network: $($interface.Name)" -ForegroundColor Yellow
        Write-Host "   IP Address: $($adapter.IPAddress)" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Use this URL on your devices:" -ForegroundColor White
        Write-Host "   http://$($adapter.IPAddress):8080" -ForegroundColor Green
        Write-Host ""
        Write-Host "   Customer Menu:    http://$($adapter.IPAddress):8080" -ForegroundColor Cyan
        Write-Host "   Staff Dashboard:  http://$($adapter.IPAddress):8080/staff/login" -ForegroundColor Cyan
        Write-Host "   Admin Panel:      http://$($adapter.IPAddress):8080/admin/login" -ForegroundColor Cyan
        Write-Host ""
    }
}

if (-not $found) {
    Write-Host "No active network connection found!" -ForegroundColor Red
    Write-Host "   Please check:" -ForegroundColor Yellow
    Write-Host "   1. Hotspot is turned ON" -ForegroundColor White
    Write-Host "   2. Network adapter is enabled" -ForegroundColor White
    Write-Host ""
}

Write-Host "================================================" -ForegroundColor Gray
Write-Host ""
