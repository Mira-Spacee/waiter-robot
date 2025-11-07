# Order Workflow - Staff Dashboard

## 🔄 New Workflow Logic

### **Order Status Flow:**

```
1. PENDING (Waiting) → Customer places order
   ↓
2. PAID (Paid) → Staff clicks "Mark as Paid" button
   ↓
3. SENT (Completed) → Staff clicks "Mark as Sent" button → Table clears
```

---

## 📊 Status Meanings

| Status | Badge Color | Meaning | Action Available |
|--------|------------|---------|------------------|
| **Pending** | 🟠 Amber/Orange | Order placed, waiting for payment | Both "Paid" and "Sent" buttons |
| **Paid** | 🟢 Green | Customer has paid, food ready to serve | "Mark as Sent" button only |
| **Sent** | ⚪ None (cleared) | Food delivered, table cleared | Order removed from view |

---

## 🎯 Button Behavior

### **When Status = Pending (Waiting):**
Two buttons appear:
1. **💵 Mark as Paid** (Left) - Changes status to "paid", stays visible
2. **🍽️ Mark as Sent** (Right) - Directly clears the table (skips paid status)

### **When Status = Paid:**
One button appears:
- **🍽️ Mark as Sent (Clear Table)** (Full width) - Clears the table

---

## 💡 Use Cases

### **Scenario 1: Normal Flow (Pay First, Serve Later)**
1. Customer orders → Status: **Pending** (Amber)
2. Customer pays at counter → Staff clicks **"Paid"** → Status: **Paid** (Green)
3. Food is ready → Staff serves → Staff clicks **"Sent"** → Table clears

### **Scenario 2: Quick Service (Serve Immediately)**
1. Customer orders → Status: **Pending** (Amber)
2. Food ready immediately → Staff clicks **"Sent"** directly → Table clears
   (Skips payment tracking - for fast service)

### **Scenario 3: Multiple Orders (Grouping)**
1. Table 3 orders burger → **Pending**
2. Staff clicks **"Paid"** → **Paid**
3. Customer orders coffee (new order) → Both show as **Pending**
4. Staff clicks **"Paid"** again → All become **Paid**
5. Staff clicks **"Sent"** → Everything clears together

---

## 🎨 Visual Indicators

### **Card Design by Status:**

**Pending (Waiting):**
- Border: Amber
- Background: Amber gradient
- Badge: "Waiting" (Orange)
- Icon ring: Amber
- Shadow: Amber glow

**Paid:**
- Border: Green
- Background: Emerald gradient
- Badge: "Paid" (Green)
- Icon ring: Green
- Shadow: Green glow

**Empty:**
- Border: Gray
- Background: Slate gradient
- Badge: "Available" (Gray)
- No icon ring

---

## 📝 Order Notes Section

All active orders for a table are grouped and shown in the **Order Notes** section:
- Scrollable list
- Shows: Quantity × Item Name → Price
- Total amount displayed below
- Custom styled scrollbar

---

## 🔧 Technical Details

### **Filter Logic:**
- Staff Dashboard shows: `status !== 'sent'`
- Admin Reports count: `status === 'sent'` (completed orders)

### **Button Actions:**
- **Paid button:** Updates status to `'paid'`
- **Sent button:** Updates status to `'sent'` (removes from staff view)

### **Order Grouping:**
- Multiple pending/paid orders on same table → All shown together
- Clicking "Paid" → Marks all pending as paid
- Clicking "Sent" → Marks all (pending + paid) as sent

---

## ✅ Benefits of This Workflow

1. **Payment tracking:** Know which tables have paid
2. **Visual clarity:** Green = money received, ready to serve
3. **Flexible:** Can skip payment step for quick service
4. **Grouped orders:** Multiple orders handled together
5. **Clean interface:** Only active orders visible

---

**Perfect for:** Restaurants where customers pay before food is served (cafeteria style, fast food, food court)
