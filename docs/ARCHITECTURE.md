# System & Web Architecture

AVIV is a **three-tier system**. People interact through a web app, a local server is the
single source of truth, and the ESP32 brain executes the physical delivery.

```
  Presentation Layer        Application Layer            Control Layer
 ┌───────────────────┐   ┌──────────────────────┐   ┌────────────────────┐
 │ Customer (QR menu) │   │  Node.js + Express   │   │  ESP32 firmware    │
 │ Staff dashboard    │◄─►│  REST API            │◄─►│  PID line follow   │
 │ Admin reports      │   │  SQLite database     │   │  S-curve motion    │
 │ Robot tablet       │   │  (orders/users)      │   │  serving algorithm │
 └───────────────────┘   └──────────────────────┘   └────────────────────┘
        React/TS                Local Wi-Fi (mDNS)         IR · ultrasonic · encoders
```

## Order lifecycle (sequence)

1. **Customer** scans a table QR code → the menu opens with the table number **locked**.
2. Customer builds a basket and confirms → `POST /api/orders` → stored in SQLite as `pending`.
3. The order surfaces on the **staff dashboard** (polled in near real time).
4. Cashier marks it **paid**; kitchen loads the food and presses **Launch AVIV**.
5. The **ESP32 polls** the API for paid orders, computes a route, and drives to the table(s).
6. While moving, AVIV follows the line (PID), smooths speed (S-curve), and stops for obstacles (ultrasonic).
7. After the last delivery, the robot returns to the staff station and reports status back to the API.

## Roles & interfaces

| Role | Can do |
|---|---|
| 👤 **Customer** | Scan QR, browse menu, manage basket, place order (table number is fixed per QR) |
| 🧑‍🍳 **Staff — Cashier** | Take manual orders if a customer device fails, mark orders paid, monitor robot path |
| 🧑‍🍳 **Staff — Kitchen** | Confirm orders, load the robot, mark "sent" / launch delivery |
| 📊 **Admin** | Analytics & revenue, most-ordered items, create/delete staff & admin accounts, archive/clear records |

## Local REST API

Every device — customer, staff, admin, and the robot — talks to one local API.

| Method & path | Purpose |
|---|---|
| `GET /api/orders` | List all orders (with line items) |
| `POST /api/orders` | Create a new order |
| `PATCH /api/orders/:id` | Update status (`pending` → `paid` → `sent`) |
| `DELETE /api/orders` | Clear all orders (admin) |
| `GET /api/health` | Backend reachability check |
| `POST /api/auth/login` | Authenticate (username + password + role) |
| `GET /api/users` | List users (passwords excluded) |

**Data flow**
- Customer → API → DB: order is validated and inserted.
- Staff ↔ API ↔ DB: orders fetched via `GET`, status changes via `PATCH`.
- Robot ← API: the ESP32 polls a dedicated endpoint for paid orders ready to deliver.
- Admin ↔ API ↔ DB: same API for user management and maintenance.

## Database (SQLite)

A single-file, zero-config database — ideal for a single-location deployment.

- **users** — `id, username, password, role (staff|admin), created_at`
- **orders** — `id, table_number, total, status, timestamp`
- **order_items** — `id, order_id → orders(id), item_id, name, price, quantity, category, image`

Indexes on `table_number`, `status`, and `timestamp` keep reads fast. The schema and a
default admin/staff account are **created automatically on first server start** — there is
no manual setup step and no database file committed to the repo.

## Local network access (mDNS)

The system uses **mDNS / Bonjour** so devices connect by hostname instead of a hard-coded IP:

- Works automatically when devices share the café Wi-Fi.
- Survives DHCP IP changes — when the ESP32 reboots and gets a new IP, it re-broadcasts
  `restaurant-esp32.local`, so the web app keeps reaching it without reconfiguration.
- Acts as a soft security boundary: communication stays inside the local network, reducing
  the risk of spoofed orders from outside.

The web app's API base URL is derived from the page host by default and can be overridden
with `VITE_API_*` environment variables (see [`../.env.example`](../.env.example)); the
robot endpoint is configured in [`../src/config/esp32.config.ts`](../src/config/esp32.config.ts).

## Frontend structure

```
src/
├── pages/          Index (menu), StaffLogin/Dashboard, AdminLogin/Reports/UserManagement, RobotTablet
├── components/     BasketSidebar, MenuItemCard, TableCard + shadcn/ui primitives
├── contexts/       OrderContext — global order state & polling
├── services/       api.service, archive.service, esp32.service
├── data/           menuItems
└── config/         esp32.config
```
