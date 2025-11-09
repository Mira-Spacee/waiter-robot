# 🍽️ Restaurant Ordering System

A modern, full-featured restaurant ordering system with real-time multi-device synchronization, ESP32 integration, staff/admin dashboards, and **secure user management**.

---

## ✨ Features

- 📱 **Customer Menu** - Browse and order from any device
- 🔄 **Real-Time Sync** - Orders sync across all devices instantly
- 📡 **ESP32 Integration** - Table notifications via WiFi-connected ESP32
- 👨‍🍳 **Staff Dashboard** - Live order tracking and status updates
- 📊 **Admin Reports** - Analytics, revenue tracking, and data management
- 🔐 **User Management** - Secure authentication with role-based access (NEW!)
- 👥 **Multi-User Support** - Add/delete admin and staff accounts (NEW!)
- 🔒 **Password Security** - SHA-256 hashed passwords (NEW!)
- 🌐 **Auto IP Detection** - Seamless WiFi/hotspot switching (NEW!)
- 📦 **Monthly Archive** - Built-in database management
- 🎨 **Modern UI** - Clean, responsive glassmorphism design

---

## 🔐 Default Login Credentials

**Admin Account:**
- Username: `admin`
- Password: `admin123`
- Access: Admin Reports + User Management

**First Steps:**
1. Login as admin
2. Go to "Manage Users"
3. Create staff accounts
4. Consider changing admin password for security

📖 **Full Documentation:** See `AUTHENTICATION_GUIDE.md`

---

## 🚀 Quick Start

### **Prerequisites**
- Node.js (v16 or higher)
- NPM
- WiFi network

### **Installation**

```powershell
# 1. Install dependencies
npm install

# 2. Install backend dependencies
cd server
npm install
cd ..
```

### **Running the System**

**Option 1: Automatic (Recommended)**
```powershell
.\start-servers.ps1
```

**Option 2: Manual (Two terminals)**

Terminal 1 - Backend:
```powershell
cd server
npm start
```

Terminal 2 - Frontend:
```powershell
npm run dev
```

### **Access URLs**

Get your computer's IP address:
```powershell
ipconfig
```
Look for "IPv4 Address" (e.g., `192.168.1.100`)

**From any device on your WiFi:**
- Customer Menu: `http://YOUR_IP:8080`
- Staff Dashboard: `http://YOUR_IP:8080/staff/login`
- Admin Panel: `http://YOUR_IP:8080/admin/login`

---

## 📡 Architecture

```
┌─────────────┐
│  Customers  │
│  (Mobile)   │──┐
└─────────────┘  │
                 │
┌─────────────┐  │    ┌──────────────┐    ┌─────────┐
│   Staff     │  ├────│  Backend API │────│  ESP32  │
│  (Tablet)   │──┤    │  Port 3001   │    │ Notifier│
└─────────────┘  │    └──────────────┘    └─────────┘
                 │           │
┌─────────────┐  │      ┌────────┐
│   Admin     │──┘      │ orders │
│  (Laptop)   │         │  .json │
└─────────────┘         └────────┘
```

### **Tech Stack**

**Frontend:**
- React + TypeScript
- Vite
- Tailwind CSS
- Shadcn/ui Components
- React Router
- Recharts (Analytics)

**Backend:**
- Node.js + Express
- File-based storage (JSON)
- CORS enabled
- RESTful API

**Hardware Integration:**
- ESP32 WiFi module
- HTTP POST notifications

---

## 🌐 Multi-Device Setup

### **How It Works**

1. **Backend Server** (Port 3001) stores all orders in `server/orders.json`
2. **Frontend App** (Port 8080) polls backend every 3 seconds
3. **All devices** see updates in real-time (within 3 seconds)

### **Network Configuration**

**Make Website Accessible on WiFi:**

1. **Get Your IP Address:**
   ```powershell
   ipconfig
   ```
   Note your IPv4 address (e.g., `192.168.1.50`)

2. **Configure API Service:**
   
   Edit `src/services/api.service.ts`:
   ```typescript
   const host = '192.168.1.50'; // ← Your computer's IP
   ```

3. **Share URL with Devices:**
   - Print QR codes for tables
   - Add to WiFi welcome page
   - Display on screens

### **Set Static IP (Optional but Recommended)**

To prevent IP from changing on restart:

**Windows:**
1. Press `Win + R`, type `ncpa.cpl`
2. Right-click WiFi adapter → Properties
3. Double-click "Internet Protocol Version 4 (TCP/IPv4)"
4. Select "Use the following IP address"
5. Enter your current IP, subnet (255.255.255.0), and gateway

OR use the PowerShell script:
```powershell
.\set_static_ip.ps1
```

---

## 📡 ESP32 Integration

### **Setup**

1. **Configure ESP32 IP**
   
   Edit `src/config/esp32.config.ts`:
   ```typescript
   export const ESP32_CONFIG = {
     ipAddress: '192.168.1.100', // ← Your ESP32's IP
     port: 80,
     endpoint: '/order',
     timeout: 5000,
   };
   ```

2. **Upload ESP32 Code**
   
   - Open `esp32_restaurant_notification.ino` in Arduino IDE
   - Update WiFi credentials (lines 23-24)
   - Upload to ESP32
   - Note the IP address from Serial Monitor
   - Update `esp32.config.ts` with ESP32's IP

3. **Test Connection**
   
   Place a test order and check ESP32 Serial Monitor for table number.

### **ESP32 Features**

- Receives table number via HTTP POST
- JSON format: `{"tableNumber": 5, "timestamp": "..."}`
- Can trigger LED, buzzer, display, etc.
- Example Arduino code included

For detailed ESP32 setup, see [`ESP32_SETUP.md`](ESP32_SETUP.md)

---

## 👥 User Roles

### **Customer**
- Browse menu (meals & drinks)
- Add items to basket
- Select table number
- Place order

### **Staff**
- View all tables
- See active orders
- Mark orders as "Sent" or "Paid"
- Real-time updates

### **Admin**
- View analytics and reports
- Track revenue and popular items
- Archive old orders
- Clear database
- Export data

---

## 📊 Admin Features

### **Analytics Dashboard**

- Total orders
- Total earnings
- Most popular items
- Order distribution charts
- Filter by period (daily/weekly/monthly)

### **Database Management**

**Archive Old Orders:**
1. Login to admin panel
2. Click "Archive Old Orders"
3. Orders older than 1 month → Moved to backup file
4. Recent orders remain for fast performance

**Clear All Orders:**
1. Click "Clear All" button
2. Confirm twice (safety)
3. All orders → Backed up to file
4. Database reset to empty

**Backup Files Location:**
```
server/
  ├── orders.json (active orders)
  ├── orders_archive_YYYY-MM-DD.json (archived)
  └── orders_backup_YYYY-MM-DD.json (full backup)
```

For detailed instructions, see [`ARCHIVE_GUIDE.md`](ARCHIVE_GUIDE.md)

---

## 🗂️ Project Structure

```
diner-dashboard-direct-main/
├── server/                    # Backend API
│   ├── index.js              # Express server
│   ├── orders.json           # Order database
│   └── package.json          # Backend dependencies
│
├── src/
│   ├── components/
│   │   ├── ui/               # Shadcn components (14 files)
│   │   ├── BasketSidebar.tsx # Shopping cart
│   │   ├── MenuItemCard.tsx  # Menu item display
│   │   └── TableCard.tsx     # Table status card
│   │
│   ├── config/
│   │   └── esp32.config.ts   # ESP32 settings
│   │
│   ├── contexts/
│   │   └── OrderContext.tsx  # Global state management
│   │
│   ├── pages/
│   │   ├── Index.tsx         # Customer menu
│   │   ├── StaffLogin.tsx    # Staff authentication
│   │   ├── StaffDashboard.tsx# Staff order view
│   │   ├── AdminLogin.tsx    # Admin authentication
│   │   ├── AdminReports.tsx  # Admin analytics
│   │   └── NotFound.tsx      # 404 page
│   │
│   ├── services/
│   │   ├── api.service.ts    # Backend API calls
│   │   ├── archive.service.ts# Archive/clear orders
│   │   └── esp32.service.ts  # ESP32 notifications
│   │
│   ├── data/
│   │   └── menuItems.ts      # Menu configuration
│   │
│   └── assets/               # Food images
│
├── public/
│   └── favicon.ico
│
├── esp32_restaurant_notification.ino  # ESP32 code
├── start-servers.ps1         # Startup script
├── set_static_ip.ps1         # Network config script
├── README.md                 # This file
├── ESP32_SETUP.md            # ESP32 guide
└── ARCHIVE_GUIDE.md          # Archive feature guide
```

---

## 🎨 Customization

### **Update Menu Items**

Edit `src/data/menuItems.ts`:

```typescript
export const menuItems = [
  {
    id: '1',
    name: 'Burger',
    price: 12.99,
    image: '/src/assets/burger.jpg',
    category: 'meals'
  },
  // Add more items...
];
```

### **Change Table Count**

Edit `src/components/BasketSidebar.tsx` (line ~110):

```typescript
{[1, 2, 3, 4, 5].map(num => ( // ← Change array for more tables
```

And `src/pages/StaffDashboard.tsx` (line ~48):

```typescript
{[1, 2, 3, 4, 5].map(tableNum => { // ← Change array
```

### **Modify Styling**

- Colors: `tailwind.config.ts`
- Global styles: `src/index.css`
- Component styles: Inline Tailwind classes

---

## 🔧 Configuration

### **Ports**

- **Frontend:** 8080 (configurable in `vite.config.ts`)
- **Backend:** 3001 (configurable in `server/index.js`)
- **ESP32:** 80 (standard HTTP)

### **Polling Interval**

Change real-time update speed in `src/contexts/OrderContext.tsx`:

```typescript
}, 3000); // ← Milliseconds (3000 = 3 seconds)
```

### **Archive Age**

Change how old orders must be to archive in `src/pages/AdminReports.tsx`:

```typescript
const result = await archiveOldOrders(1); // ← Months
```

---

## 🐛 Troubleshooting

### **Orders Not Syncing Between Devices**

✅ Check both servers are running (ports 3001 & 8080)
✅ Verify all devices on same WiFi
✅ Check API service has correct IP address
✅ Check browser console for errors (F12)

### **ESP32 Not Receiving Notifications**

✅ ESP32 IP correct in `src/config/esp32.config.ts`
✅ ESP32 server running (check Serial Monitor)
✅ ESP32 on same WiFi as computer
✅ Test ESP32 endpoint: `http://ESP32_IP/ping`

### **Can't Access from Phone**

✅ Phone on same WiFi network
✅ Using `http://YOUR_IP:8080` not `localhost`
✅ Firewall allowing connections on port 8080
✅ Servers are running

### **Port Already in Use**

```powershell
# Kill process on port 8080
Get-NetTCPConnection -LocalPort 8080 | Select-Object -ExpandProperty OwningProcess | Stop-Process -Force

# Kill process on port 3001
Get-NetTCPConnection -LocalPort 3001 | Select-Object -ExpandProperty OwningProcess | Stop-Process -Force
```

### **Old Orders Not Archiving**

✅ Backend server running
✅ Check `server/` folder permissions
✅ Look for backup files in `server/` directory

---

## 🔒 Security

### **Network Security**

- ✅ Local WiFi only (not public internet)
- ✅ No external access without port forwarding
- ✅ Router firewall protects by default

### **Access Control**

- Staff/Admin require login (sessionStorage)
- Customers have direct menu access
- No user data stored (session-based auth)

### **Data Privacy**

- All data stored locally on your computer
- No cloud services or external APIs
- Orders saved in plain JSON (easy to manage)

---

## 📦 Backup & Maintenance

### **Manual Backup**

```powershell
# Backup orders
Copy-Item server\orders.json server\orders_backup.json

# Backup entire project
Copy-Item -Path . -Destination ..\diner-backup -Recurse
```

### **Monthly Maintenance**

1. Login to admin dashboard
2. Click "Archive Old Orders"
3. Check `server/` for archive files
4. Optional: Move old archives to external storage

### **Restore from Backup**

```powershell
# Restore orders
Copy-Item server\orders_backup_2025-10-09.json server\orders.json

# Restart backend
cd server
npm start
```

---

## 🚀 Deployment

### **Production Recommendations**

For production use:

1. **Database:** Consider SQLite or PostgreSQL for high volume
2. **Authentication:** Add proper user management
3. **HTTPS:** Use SSL certificates
4. **Monitoring:** Add logging and error tracking
5. **Backups:** Automated daily backups

Current setup is perfect for:
- Small to medium restaurants
- < 1000 orders/month
- Local WiFi operation
- Single location

---

## 📄 License

This project is open source and available for personal and commercial use.

---

## 🆘 Support

For issues or questions:

1. Check documentation files
2. Review error messages in console
3. Verify network configuration
4. Test with minimal setup

---

## 📚 Additional Documentation

- **ESP32 Setup:** [`ESP32_SETUP.md`](ESP32_SETUP.md) - Hardware integration guide
- **Archive Guide:** [`ARCHIVE_GUIDE.md`](ARCHIVE_GUIDE.md) - Database management
- **Arduino Code:** `esp32_restaurant_notification.ino` - ESP32 firmware

---

**Built with ❤️ for efficient restaurant operations**

🍔 Happy ordering! 🥤
