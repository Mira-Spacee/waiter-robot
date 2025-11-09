const Database = require('better-sqlite3');
const path = require('path');

// Create/open database
const dbPath = path.join(__dirname, 'restaurant.db');
const db = new Database(dbPath);

// Enable foreign keys
db.pragma('foreign_keys = ON');

// Create tables
const createTables = () => {
  // Users table
  db.exec(`
    CREATE TABLE IF NOT EXISTS users (
      id TEXT PRIMARY KEY,
      username TEXT UNIQUE NOT NULL,
      password TEXT NOT NULL,
      role TEXT NOT NULL CHECK(role IN ('staff', 'admin')),
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  // Orders table
  db.exec(`
    CREATE TABLE IF NOT EXISTS orders (
      id TEXT PRIMARY KEY,
      table_number INTEGER NOT NULL,
      total REAL NOT NULL,
      status TEXT NOT NULL CHECK(status IN ('pending', 'paid', 'sent')),
      timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  // Order items table
  db.exec(`
    CREATE TABLE IF NOT EXISTS order_items (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      order_id TEXT NOT NULL,
      item_id TEXT NOT NULL,
      name TEXT NOT NULL,
      price REAL NOT NULL,
      quantity INTEGER NOT NULL,
      category TEXT,
      image TEXT,
      FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE
    )
  `);

  // Create indexes for better performance
  db.exec(`
    CREATE INDEX IF NOT EXISTS idx_orders_table_number ON orders(table_number);
    CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
    CREATE INDEX IF NOT EXISTS idx_orders_timestamp ON orders(timestamp);
    CREATE INDEX IF NOT EXISTS idx_order_items_order_id ON order_items(order_id);
  `);

  console.log('✅ Database tables created successfully!');
};

// Initialize database with default users (if empty)
const seedDefaultUsers = () => {
  const userCount = db.prepare('SELECT COUNT(*) as count FROM users').get();
  
  if (userCount.count === 0) {
    const insertUser = db.prepare('INSERT INTO users (id, username, password, role) VALUES (?, ?, ?, ?)');
    
    // Default users (same as before)
    insertUser.run('staff1', 'staff', 'staff123', 'staff');
    insertUser.run('admin1', 'admin', 'admin123', 'admin');
    
    console.log('✅ Default users created!');
  }
};

// Migrate existing JSON data to SQLite (if exists)
const migrateFromJSON = () => {
  try {
    const fs = require('fs');
    const ordersPath = path.join(__dirname, 'orders.json');
    const usersPath = path.join(__dirname, 'users.json');

    // Migrate orders
    if (fs.existsSync(ordersPath)) {
      const ordersData = JSON.parse(fs.readFileSync(ordersPath, 'utf-8'));
      const insertOrder = db.prepare('INSERT OR IGNORE INTO orders (id, table_number, total, status, timestamp) VALUES (?, ?, ?, ?, ?)');
      const insertItem = db.prepare('INSERT INTO order_items (order_id, item_id, name, price, quantity, category, image) VALUES (?, ?, ?, ?, ?, ?, ?)');
      
      const migrate = db.transaction((orders) => {
        for (const order of orders) {
          insertOrder.run(order.id, order.tableNumber, order.total, order.status, order.timestamp);
          
          for (const item of order.items) {
            insertItem.run(order.id, item.id, item.name, item.price, item.quantity, item.category || null, item.image || null);
          }
        }
      });

      migrate(ordersData);
      console.log(`✅ Migrated ${ordersData.length} orders from JSON to SQLite!`);
      
      // Backup and remove old JSON file
      fs.renameSync(ordersPath, ordersPath + '.backup');
    }

    // Migrate users
    if (fs.existsSync(usersPath)) {
      const usersData = JSON.parse(fs.readFileSync(usersPath, 'utf-8'));
      const insertUser = db.prepare('INSERT OR IGNORE INTO users (id, username, password, role) VALUES (?, ?, ?, ?)');
      
      for (const user of usersData) {
        insertUser.run(user.id, user.username, user.password, user.role);
      }
      
      console.log(`✅ Migrated ${usersData.length} users from JSON to SQLite!`);
      
      // Backup and remove old JSON file
      fs.renameSync(usersPath, usersPath + '.backup');
    }
  } catch (error) {
    console.log('ℹ️ No existing JSON data to migrate or migration already done');
  }
};

// Initialize database
const initDatabase = () => {
  console.log('\n========================================');
  console.log('🗄️  Initializing SQLite Database');
  console.log('========================================\n');
  
  createTables();
  seedDefaultUsers();
  migrateFromJSON();
  
  console.log('\n========================================');
  console.log('✅ Database ready at:', dbPath);
  console.log('========================================\n');
};

// Run initialization
initDatabase();

module.exports = db;
