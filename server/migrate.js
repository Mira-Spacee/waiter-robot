const db = require('./database');
const fs = require('fs');
const path = require('path');

console.log('\n========================================');
console.log('🔄 Migrating All JSON Data to SQLite');
console.log('========================================\n');

// Function to migrate JSON files
const migrateAllJSON = () => {
  const serverDir = __dirname;
  
  // Find all JSON files
  const jsonFiles = fs.readdirSync(serverDir)
    .filter(file => file.endsWith('.json') && !file.includes('package'));
  
  // Also check for .backup files
  const backupFiles = fs.readdirSync(serverDir)
    .filter(file => file.endsWith('.backup'));
  
  console.log('📁 Found files:', [...jsonFiles, ...backupFiles]);
  
  let totalOrdersMigrated = 0;
  let totalUsersMigrated = 0;
  
  // Migrate orders from all sources
  const orderFiles = [...jsonFiles, ...backupFiles].filter(f => 
    f.includes('order') || f === 'orders.json' || f === 'orders.json.backup'
  );
  
  for (const file of orderFiles) {
    try {
      const filePath = path.join(serverDir, file);
      const data = JSON.parse(fs.readFileSync(filePath, 'utf-8'));
      
      if (Array.isArray(data) && data.length > 0) {
        console.log(`\n📦 Processing: ${file} (${data.length} orders)`);
        
        const insertOrder = db.prepare(
          'INSERT OR IGNORE INTO orders (id, table_number, total, status, timestamp) VALUES (?, ?, ?, ?, ?)'
        );
        
        const insertItem = db.prepare(
          'INSERT OR IGNORE INTO order_items (order_id, item_id, name, price, quantity, category, image) VALUES (?, ?, ?, ?, ?, ?, ?)'
        );
        
        const migrate = db.transaction((orders) => {
          for (const order of orders) {
            try {
              insertOrder.run(
                order.id,
                order.tableNumber,
                order.total,
                order.status || 'pending',
                order.timestamp
              );
              
              if (order.items && Array.isArray(order.items)) {
                for (const item of order.items) {
                  insertItem.run(
                    order.id,
                    item.id,
                    item.name,
                    item.price,
                    item.quantity,
                    item.category || null,
                    item.image || null
                  );
                }
              }
              totalOrdersMigrated++;
            } catch (err) {
              console.log(`  ⚠️  Skipped duplicate order: ${order.id}`);
            }
          }
        });
        
        migrate(data);
        console.log(`  ✅ Migrated ${data.length} orders from ${file}`);
      }
    } catch (error) {
      console.log(`  ⚠️  Could not process ${file}:`, error.message);
    }
  }
  
  // Migrate users from all sources
  const userFiles = [...jsonFiles, ...backupFiles].filter(f => 
    f.includes('user') || f === 'users.json' || f === 'users.json.backup'
  );
  
  for (const file of userFiles) {
    try {
      const filePath = path.join(serverDir, file);
      const data = JSON.parse(fs.readFileSync(filePath, 'utf-8'));
      
      if (Array.isArray(data) && data.length > 0) {
        console.log(`\n👥 Processing: ${file} (${data.length} users)`);
        
        const insertUser = db.prepare(
          'INSERT OR IGNORE INTO users (id, username, password, role, created_at) VALUES (?, ?, ?, ?, ?)'
        );
        
        for (const user of data) {
          try {
            insertUser.run(
              user.id,
              user.username,
              user.password,
              user.role,
              user.createdAt || new Date().toISOString()
            );
            totalUsersMigrated++;
          } catch (err) {
            console.log(`  ⚠️  Skipped duplicate user: ${user.username}`);
          }
        }
        
        console.log(`  ✅ Migrated ${data.length} users from ${file}`);
      }
    } catch (error) {
      console.log(`  ⚠️  Could not process ${file}:`, error.message);
    }
  }
  
  console.log('\n========================================');
  console.log('📊 Migration Summary:');
  console.log(`   Orders: ${totalOrdersMigrated}`);
  console.log(`   Users: ${totalUsersMigrated}`);
  console.log('========================================');
  
  // Show current database stats
  const orderCount = db.prepare('SELECT COUNT(*) as count FROM orders').get();
  const userCount = db.prepare('SELECT COUNT(*) as count FROM users').get();
  const itemCount = db.prepare('SELECT COUNT(*) as count FROM order_items').get();
  
  console.log('\n📈 Current Database Statistics:');
  console.log(`   Total Orders: ${orderCount.count}`);
  console.log(`   Total Users: ${userCount.count}`);
  console.log(`   Total Order Items: ${itemCount.count}`);
  console.log('========================================\n');
};

// Run migration
migrateAllJSON();
