const express = require('express');
const cors = require('cors');
const db = require('./database');
const { hashPassword } = require('./passwordUtils');

const app = express();
const PORT = 3001;

// Enable CORS for all devices on local network
app.use(cors());
app.use(express.json());

// ============================================
// ORDER API ROUTES (Using SQLite)
// ============================================

// Get all orders
app.get('/api/orders', (req, res) => {
  try {
    const orders = db.prepare(`
      SELECT o.*, GROUP_CONCAT(
        json_object(
          'id', oi.item_id,
          'name', oi.name,
          'price', oi.price,
          'quantity', oi.quantity,
          'category', oi.category,
          'image', oi.image
        )
      ) as items_json
      FROM orders o
      LEFT JOIN order_items oi ON o.id = oi.order_id
      GROUP BY o.id
      ORDER BY o.timestamp DESC
    `).all();

    // Parse items JSON
    const ordersWithItems = orders.map(order => ({
      id: order.id,
      tableNumber: order.table_number,
      total: order.total,
      status: order.status,
      timestamp: order.timestamp,
      items: order.items_json ? JSON.parse(`[${order.items_json}]`) : []
    }));

    res.json(ordersWithItems);
  } catch (error) {
    console.error('Error fetching orders:', error);
    res.status(500).json({ success: false, error: 'Failed to fetch orders' });
  }
});

// Add new order
app.post('/api/orders', (req, res) => {
  try {
    const { id, tableNumber, items, total, status, timestamp } = req.body;

    const insertOrder = db.prepare(
      'INSERT INTO orders (id, table_number, total, status, timestamp) VALUES (?, ?, ?, ?, ?)'
    );
    
    const insertItem = db.prepare(
      'INSERT INTO order_items (order_id, item_id, name, price, quantity, category, image) VALUES (?, ?, ?, ?, ?, ?, ?)'
    );

    const transaction = db.transaction(() => {
      insertOrder.run(id, tableNumber, total, status, timestamp);
      
      for (const item of items) {
        insertItem.run(
          id,
          item.id,
          item.name,
          item.price,
          item.quantity,
          item.category || null,
          item.image || null
        );
      }
    });

    transaction();
    
    res.status(201).json({ success: true, order: req.body });
  } catch (error) {
    console.error('Error creating order:', error);
    res.status(500).json({ success: false, error: 'Failed to save order' });
  }
});

// Update order status
app.patch('/api/orders/:id', (req, res) => {
  try {
    const { id } = req.params;
    const { status } = req.body;

    const updateOrder = db.prepare('UPDATE orders SET status = ? WHERE id = ?');
    const result = updateOrder.run(status, id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    const order = db.prepare('SELECT * FROM orders WHERE id = ?').get(id);
    res.json({ success: true, order });
  } catch (error) {
    console.error('Error updating order:', error);
    res.status(500).json({ success: false, error: 'Failed to update order' });
  }
});

// Clear all orders
app.delete('/api/orders', (req, res) => {
  try {
    db.prepare('DELETE FROM orders').run();
    res.json({ success: true, message: 'All orders cleared' });
  } catch (error) {
    console.error('Error clearing orders:', error);
    res.status(500).json({ success: false, error: 'Failed to clear orders' });
  }
});

// Health check
app.get('/api/health', (req, res) => {
  res.json({ status: 'OK', timestamp: new Date().toISOString() });
});

// ============================================
// USER MANAGEMENT API ROUTES (Using SQLite)
// ============================================

// Login (authenticate user)
app.post('/api/auth/login', (req, res) => {
  try {
    const { username, password, role } = req.body;
    
    if (!username || !password || !role) {
      return res.status(400).json({ success: false, error: 'Missing credentials' });
    }
    
    const hashedPassword = hashPassword(password);
    
    const user = db.prepare('SELECT * FROM users WHERE username = ? AND password = ? AND role = ?')
      .get(username, hashedPassword, role);
    
    if (user) {
      const { password: _, ...userWithoutPassword } = user;
      res.json({ success: true, user: userWithoutPassword });
    } else {
      res.status(401).json({ success: false, error: 'Invalid credentials' });
    }
  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({ success: false, error: 'Login failed' });
  }
});

// Get all users
app.get('/api/users', (req, res) => {
  try {
    const users = db.prepare('SELECT id, username, role, created_at FROM users').all();
    res.json(users);
  } catch (error) {
    console.error('Get users error:', error);
    res.status(500).json({ success: false, error: 'Failed to get users' });
  }
});

// Create new user
app.post('/api/users', (req, res) => {
  try {
    const { username, password, role } = req.body;
    
    if (!username || !password || !role) {
      return res.status(400).json({ success: false, error: 'Missing user data' });
    }
    
    if (role !== 'admin' && role !== 'staff') {
      return res.status(400).json({ success: false, error: 'Invalid role' });
    }
    
    const existingUser = db.prepare('SELECT id FROM users WHERE username = ?').get(username);
    if (existingUser) {
      return res.status(409).json({ success: false, error: 'Username already exists' });
    }
    
    const id = Date.now().toString();
    const hashedPassword = hashPassword(password);
    
    db.prepare('INSERT INTO users (id, username, password, role) VALUES (?, ?, ?, ?)')
      .run(id, username, hashedPassword, role);
    
    const newUser = { id, username, role, created_at: new Date().toISOString() };
    res.status(201).json({ success: true, user: newUser });
  } catch (error) {
    console.error('Create user error:', error);
    res.status(500).json({ success: false, error: 'Failed to create user' });
  }
});

// Delete user
app.delete('/api/users/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const user = db.prepare('SELECT * FROM users WHERE id = ?').get(id);
    
    if (!user) {
      return res.status(404).json({ success: false, error: 'User not found' });
    }
    
    if (user.role === 'admin') {
      const adminCount = db.prepare('SELECT COUNT(*) as count FROM users WHERE role = ?').get('admin');
      if (adminCount.count <= 1) {
        return res.status(400).json({ success: false, error: 'Cannot delete the last admin' });
      }
    }
    
    db.prepare('DELETE FROM users WHERE id = ?').run(id);
    res.json({ success: true, message: 'User deleted successfully' });
  } catch (error) {
    console.error('Delete user error:', error);
    res.status(500).json({ success: false, error: 'Failed to delete user' });
  }
});

// Start server
app.listen(PORT, '0.0.0.0', () => {
  console.log('\n========================================');
  console.log('🍽️  Restaurant Order Server Started');
  console.log('========================================');
  console.log(`📡 Server running on port ${PORT}`);
  console.log(`🌐 Accessible at:`);
  console.log(`   - Local: http://localhost:${PORT}`);
  console.log(`   - Network: http://YOUR_IP:${PORT}`);
  console.log('========================================');
  console.log('💾 Database: SQLite (restaurant.db)');
  console.log('========================================');
  console.log('Order API:');
  console.log(`   GET    /api/orders       - Get all orders`);
  console.log(`   POST   /api/orders       - Create new order`);
  console.log(`   PATCH  /api/orders/:id   - Update order status`);
  console.log(`   DELETE /api/orders       - Clear all orders`);
  console.log('User Management API:');
  console.log(`   POST   /api/auth/login   - User login`);
  console.log(`   GET    /api/users        - Get all users`);
  console.log(`   POST   /api/users        - Create new user`);
  console.log(`   DELETE /api/users/:id    - Delete user`);
  console.log('========================================\n');
});
