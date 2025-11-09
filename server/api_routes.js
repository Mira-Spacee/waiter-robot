const express = require('express');
const db = require('./database');

const router = express.Router();

// ============================================
// ESP32 API ROUTES
// ============================================

// ESP32 - Get pending orders (orders that are paid but not sent)
router.get('/esp32/pending-orders', (req, res) => {
  try {
    const orders = db.prepare(`
      SELECT o.*, GROUP_CONCAT(
        json_object(
          'id', oi.item_id,
          'name', oi.name,
          'price', oi.price,
          'quantity', oi.quantity
        )
      ) as items_json
      FROM orders o
      LEFT JOIN order_items oi ON o.id = oi.order_id
      WHERE o.status = 'paid'
      GROUP BY o.id
      ORDER BY o.timestamp ASC
    `).all();

    const ordersWithItems = orders.map(order => ({
      id: order.id,
      tableNumber: order.table_number,
      total: order.total,
      status: order.status,
      timestamp: order.timestamp,
      items: order.items_json ? JSON.parse(`[${order.items_json}]`) : []
    }));

    res.json({
      success: true,
      count: ordersWithItems.length,
      orders: ordersWithItems
    });
  } catch (error) {
    console.error('ESP32 pending orders error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ESP32 - Get specific order by ID
router.get('/esp32/order/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const order = db.prepare(`
      SELECT o.*, GROUP_CONCAT(
        json_object(
          'id', oi.item_id,
          'name', oi.name,
          'price', oi.price,
          'quantity', oi.quantity
        )
      ) as items_json
      FROM orders o
      LEFT JOIN order_items oi ON o.id = oi.order_id
      WHERE o.id = ?
      GROUP BY o.id
    `).get(id);

    if (!order) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    const orderData = {
      id: order.id,
      tableNumber: order.table_number,
      total: order.total,
      status: order.status,
      timestamp: order.timestamp,
      items: order.items_json ? JSON.parse(`[${order.items_json}]`) : []
    };

    res.json({ success: true, order: orderData });
  } catch (error) {
    console.error('ESP32 get order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ESP32 - Update robot status (arrived, delivering, etc.)
router.post('/esp32/status', (req, res) => {
  try {
    const { orderId, status, location, battery } = req.body;

    // You can create a robot_status table if needed
    // For now, just log and return success
    console.log('ESP32 Status Update:', { orderId, status, location, battery });

    res.json({
      success: true,
      message: 'Status updated',
      timestamp: new Date().toISOString()
    });
  } catch (error) {
    console.error('ESP32 status update error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ESP32 - Mark order as delivered
router.post('/esp32/delivered/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const result = db.prepare('UPDATE orders SET status = ? WHERE id = ?')
      .run('sent', id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    res.json({
      success: true,
      message: 'Order marked as delivered',
      orderId: id
    });
  } catch (error) {
    console.error('ESP32 delivered error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ESP32 - Ping/Health check
router.get('/esp32/ping', (req, res) => {
  res.json({
    success: true,
    message: 'ESP32 API Online',
    timestamp: new Date().toISOString()
  });
});

// ============================================
// ORDERS - Full CRUD
// ============================================

// CREATE - Add new order
router.post('/orders', (req, res) => {
  try {
    const { id, tableNumber, items, total, status, timestamp } = req.body;

    const insertOrder = db.prepare(
      'INSERT INTO orders (id, table_number, total, status, timestamp) VALUES (?, ?, ?, ?, ?)'
    );
    
    const insertItem = db.prepare(
      'INSERT INTO order_items (order_id, item_id, name, price, quantity, category, image) VALUES (?, ?, ?, ?, ?, ?, ?)'
    );

    const transaction = db.transaction(() => {
      insertOrder.run(id, tableNumber, total, status || 'pending', timestamp);
      
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
    console.error('Create order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// READ - Get all orders
router.get('/orders', (req, res) => {
  try {
    const { status, tableNumber, limit } = req.query;
    
    let query = `
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
    `;
    
    const conditions = [];
    const params = [];
    
    if (status) {
      conditions.push('o.status = ?');
      params.push(status);
    }
    
    if (tableNumber) {
      conditions.push('o.table_number = ?');
      params.push(parseInt(tableNumber));
    }
    
    if (conditions.length > 0) {
      query += ' WHERE ' + conditions.join(' AND ');
    }
    
    query += ' GROUP BY o.id ORDER BY o.timestamp DESC';
    
    if (limit) {
      query += ' LIMIT ?';
      params.push(parseInt(limit));
    }

    const orders = db.prepare(query).all(...params);

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
    console.error('Get orders error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// READ - Get single order by ID
router.get('/orders/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const order = db.prepare(`
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
      WHERE o.id = ?
      GROUP BY o.id
    `).get(id);

    if (!order) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    const orderData = {
      id: order.id,
      tableNumber: order.table_number,
      total: order.total,
      status: order.status,
      timestamp: order.timestamp,
      items: order.items_json ? JSON.parse(`[${order.items_json}]`) : []
    };

    res.json(orderData);
  } catch (error) {
    console.error('Get order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// UPDATE - Update order status
router.patch('/orders/:id', (req, res) => {
  try {
    const { id } = req.params;
    const { status } = req.body;

    const result = db.prepare('UPDATE orders SET status = ? WHERE id = ?')
      .run(status, id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    const order = db.prepare('SELECT * FROM orders WHERE id = ?').get(id);
    res.json({ success: true, order });
  } catch (error) {
    console.error('Update order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// UPDATE - Update entire order
router.put('/orders/:id', (req, res) => {
  try {
    const { id } = req.params;
    const { tableNumber, total, status } = req.body;

    const result = db.prepare(
      'UPDATE orders SET table_number = ?, total = ?, status = ? WHERE id = ?'
    ).run(tableNumber, total, status, id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    const order = db.prepare('SELECT * FROM orders WHERE id = ?').get(id);
    res.json({ success: true, order });
  } catch (error) {
    console.error('Update order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// DELETE - Delete single order
router.delete('/orders/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const result = db.prepare('DELETE FROM orders WHERE id = ?').run(id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Order not found' });
    }

    res.json({ success: true, message: 'Order deleted', deletedId: id });
  } catch (error) {
    console.error('Delete order error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// DELETE - Clear all orders
router.delete('/orders', (req, res) => {
  try {
    const result = db.prepare('DELETE FROM orders').run();
    res.json({ 
      success: true, 
      message: 'All orders cleared',
      deletedCount: result.changes
    });
  } catch (error) {
    console.error('Clear orders error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ============================================
// ORDER ITEMS - Full CRUD
// ============================================

// READ - Get all items for an order
router.get('/order-items/:orderId', (req, res) => {
  try {
    const { orderId } = req.params;
    
    const items = db.prepare(`
      SELECT * FROM order_items WHERE order_id = ?
    `).all(orderId);

    res.json({ success: true, items });
  } catch (error) {
    console.error('Get order items error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// CREATE - Add item to order
router.post('/order-items', (req, res) => {
  try {
    const { orderId, itemId, name, price, quantity, category, image } = req.body;

    const result = db.prepare(
      'INSERT INTO order_items (order_id, item_id, name, price, quantity, category, image) VALUES (?, ?, ?, ?, ?, ?, ?)'
    ).run(orderId, itemId, name, price, quantity, category || null, image || null);

    res.status(201).json({ 
      success: true, 
      itemId: result.lastInsertRowid 
    });
  } catch (error) {
    console.error('Create order item error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// UPDATE - Update order item
router.put('/order-items/:id', (req, res) => {
  try {
    const { id } = req.params;
    const { quantity, price } = req.body;

    const result = db.prepare(
      'UPDATE order_items SET quantity = ?, price = ? WHERE id = ?'
    ).run(quantity, price, id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Item not found' });
    }

    res.json({ success: true, message: 'Item updated' });
  } catch (error) {
    console.error('Update order item error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// DELETE - Delete order item
router.delete('/order-items/:id', (req, res) => {
  try {
    const { id } = req.params;
    
    const result = db.prepare('DELETE FROM order_items WHERE id = ?').run(id);

    if (result.changes === 0) {
      return res.status(404).json({ success: false, error: 'Item not found' });
    }

    res.json({ success: true, message: 'Item deleted' });
  } catch (error) {
    console.error('Delete order item error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// ============================================
// USERS - Full CRUD
// ============================================

// CREATE - Add new user
router.post('/users', (req, res) => {
  try {
    const { username, password, role } = req.body;
    const crypto = require('crypto');
    
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
    const hashedPassword = crypto.createHash('sha256').update(password).digest('hex');
    
    db.prepare('INSERT INTO users (id, username, password, role) VALUES (?, ?, ?, ?)')
      .run(id, username, hashedPassword, role);
    
    const newUser = { id, username, role, created_at: new Date().toISOString() };
    res.status(201).json({ success: true, user: newUser });
  } catch (error) {
    console.error('Create user error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// READ - Get all users
router.get('/users', (req, res) => {
  try {
    const users = db.prepare('SELECT id, username, role, created_at FROM users').all();
    res.json(users);
  } catch (error) {
    console.error('Get users error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// READ - Get single user
router.get('/users/:id', (req, res) => {
  try {
    const { id } = req.params;
    const user = db.prepare('SELECT id, username, role, created_at FROM users WHERE id = ?').get(id);
    
    if (!user) {
      return res.status(404).json({ success: false, error: 'User not found' });
    }
    
    res.json(user);
  } catch (error) {
    console.error('Get user error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// UPDATE - Update user
router.put('/users/:id', (req, res) => {
  try {
    const { id } = req.params;
    const { username, role, password } = req.body;
    const crypto = require('crypto');
    
    const user = db.prepare('SELECT * FROM users WHERE id = ?').get(id);
    if (!user) {
      return res.status(404).json({ success: false, error: 'User not found' });
    }
    
    let query = 'UPDATE users SET username = ?, role = ?';
    let params = [username || user.username, role || user.role];
    
    if (password) {
      const hashedPassword = crypto.createHash('sha256').update(password).digest('hex');
      query += ', password = ?';
      params.push(hashedPassword);
    }
    
    query += ' WHERE id = ?';
    params.push(id);
    
    db.prepare(query).run(...params);
    
    const updatedUser = db.prepare('SELECT id, username, role, created_at FROM users WHERE id = ?').get(id);
    res.json({ success: true, user: updatedUser });
  } catch (error) {
    console.error('Update user error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// DELETE - Delete user
router.delete('/users/:id', (req, res) => {
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
    res.status(500).json({ success: false, error: error.message });
  }
});

// ============================================
// STATISTICS & REPORTS
// ============================================

// Get statistics
router.get('/stats', (req, res) => {
  try {
    const totalOrders = db.prepare('SELECT COUNT(*) as count FROM orders').get();
    const totalRevenue = db.prepare('SELECT SUM(total) as revenue FROM orders WHERE status = "sent"').get();
    const pendingOrders = db.prepare('SELECT COUNT(*) as count FROM orders WHERE status = "pending"').get();
    const paidOrders = db.prepare('SELECT COUNT(*) as count FROM orders WHERE status = "paid"').get();
    
    const topItems = db.prepare(`
      SELECT name, SUM(quantity) as total_sold, SUM(quantity * price) as revenue
      FROM order_items
      GROUP BY name
      ORDER BY total_sold DESC
      LIMIT 5
    `).all();
    
    res.json({
      success: true,
      stats: {
        totalOrders: totalOrders.count,
        totalRevenue: totalRevenue.revenue || 0,
        pendingOrders: pendingOrders.count,
        paidOrders: paidOrders.count,
        topItems
      }
    });
  } catch (error) {
    console.error('Get stats error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

module.exports = router;
