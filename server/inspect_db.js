const db = require('./database');

console.log('\n========================================');
console.log('🗄️  SQLite Database Inspection');
console.log('========================================\n');

// Show all users
console.log('👥 USERS:');
console.log('========================================');
const users = db.prepare('SELECT id, username, role, created_at FROM users').all();
console.table(users);

// Show all orders
console.log('\n📦 ORDERS:');
console.log('========================================');
const orders = db.prepare(`
  SELECT 
    o.id,
    o.table_number,
    o.total,
    o.status,
    o.timestamp,
    COUNT(oi.id) as item_count
  FROM orders o
  LEFT JOIN order_items oi ON o.id = oi.order_id
  GROUP BY o.id
  ORDER BY o.timestamp DESC
`).all();
console.table(orders);

// Show all order items
console.log('\n🍽️  ORDER ITEMS:');
console.log('========================================');
const items = db.prepare(`
  SELECT 
    oi.order_id,
    oi.name,
    oi.quantity,
    oi.price,
    (oi.quantity * oi.price) as subtotal
  FROM order_items oi
`).all();
console.table(items);

console.log('\n========================================');
console.log('✅ All data successfully in SQLite!');
console.log('========================================\n');
